#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/HPMi.h"
#include "../common/mmo.h"
#include "../common/malloc.h"
#include "../map/pc.h"
#include "../map/atcommand.h"
#include "../map/itemdb.h"

HPExport struct hplugin_info pinfo = {
	"autolootype",			// Plugin name
	SERVER_TYPE_MAP,	// Which server types this plugin works with?
	"1.0",				// Plugin version
	HPM_VERSION,		// HPM Version (don't change, macro is automatically updated)
};

static char custom_atcmd_output[CHAT_SIZE_MAX];

/// Returns human readable name for given item type.
/// @param type Type id to retrieve name for ( IT_* ).
const char* custom_itemdb_typename(int type)
{
	switch(type)
	{
		case IT_HEALING:        return "Potion/Food";
		case IT_USABLE:         return "Usable";
		case IT_ETC:            return "Etc.";
		case IT_WEAPON:         return "Weapon";
		case IT_ARMOR:          return "Armor";
		case IT_CARD:           return "Card";
		case IT_PETEGG:         return "Pet Egg";
		case IT_PETARMOR:       return "Pet Accessory";
		case IT_AMMO:           return "Arrow/Ammunition";
		case IT_DELAYCONSUME:   return "Delay-Consume Usable";
		case IT_CASH:           return "Cash Usable";
	}
	return "Unknown Type";
}

struct autoloottype_data_struct {
	unsigned short autoloottype;
	unsigned int autolootingtype : 1; //performance-saver, autolooting state for @autoloottype
};

struct autoloottype_data_struct *get_autoloottype_variable(struct map_session_data *sd) {
	struct autoloottype_data_struct *data;
	
	// Let's check if we already inserted the field
	if ( !(data = HPMi->getFromMSD(sd, HPMi->pid, 0)) ) {
		
		CREATE(data,struct autoloottype_data_struct, 1);
		
		HPMi->addToMSD(sd, data, HPMi->pid, 0, true);
	}

	return data;
}

bool my_pc_isautolooting_post(bool retVal, struct map_session_data *sd, int *nameid) {
	int i;
	struct autoloottype_data_struct *data = get_autoloottype_variable(sd);

	if (retVal)
		return true;

	if (data->autoloottype && (data->autoloottype)&(1<<itemdb_type(*nameid)))
		return true;

	return false;
 }

/*==========================================
 * @autoloottype
 * Flags:
 * 1:   IT_HEALING,  2:   IT_UNKNOWN,  4:    IT_USABLE, 8:    IT_ETC,
 * 16:  IT_WEAPON,   32:  IT_ARMOR,    64:   IT_CARD,   128:  IT_PETEGG,
 * 256: IT_PETARMOR, 512: IT_UNKNOWN2, 1024: IT_AMMO,   2048: IT_DELAYCONSUME
 * 262144: IT_CASH
 *------------------------------------------
 * Credits:
 *    chriser
 *    Aleos
 *------------------------------------------*/
ACMD(autoloottype)
{

	uint8 i = 0, action = 3; // 1=add, 2=remove, 3=help+list (default), 4=reset
	enum item_types type = -1;
	int ITEM_NONE = 0, ITEM_MAX = 1533;
	struct autoloottype_data_struct *my = get_autoloottype_variable(sd);

	if (!sd) return false;

	if (message && *message) {
		if (message[0] == '+') {
			message++;
			action = 1;
		}
		else if (message[0] == '-') {
			message++;
			action = 2;
		}
		else if (!strcmp(message,"reset"))
			action = 4;
	}

	if (action < 3) { // add or remove
		if ((strncmp(message, "healing", 3) == 0) || (atoi(message) == 0))
			type = IT_HEALING;
		else if ((strncmp(message, "usable", 3) == 0) || (atoi(message) == 2))
			type = IT_USABLE;
		else if ((strncmp(message, "etc", 3) == 0) || (atoi(message) == 3))
			type = IT_ETC;
		else if ((strncmp(message, "weapon", 3) == 0) || (atoi(message) == 4))
			type = IT_WEAPON;
		else if ((strncmp(message, "armor", 3) == 0) || (atoi(message) == 5))
			type = IT_ARMOR;
		else if ((strncmp(message, "card", 3) == 0) || (atoi(message) == 6))
			type = IT_CARD;
		else if ((strncmp(message, "petegg", 4) == 0) || (atoi(message) == 7))
			type = IT_PETEGG;
		else if ((strncmp(message, "petarmor", 4) == 0) || (atoi(message) == 8))
			type = IT_PETARMOR;
		else if ((strncmp(message, "ammo", 3) == 0) || (atoi(message) == 10))
			type = IT_AMMO;
		else {
			clif->message(fd,"Item type not found."); // Item type not found.
			return -1;
		}
	}

	switch (action) {
		case 1:
			if (my->autoloottype&(1<<type)) {
				clif->message(fd,"You're already autolooting this item type."); // You're already autolooting this item type.
				return -1;
			}
			if (my->autoloottype == ITEM_MAX) {
				clif->message(fd,"Your autoloottype list has all item types. You can remove some items with @autoloottype -<type name or ID>."); // Your autoloottype list has all item types. You can remove some items with @autoloottype -<type name or ID>.
				return -1;
			}
			(my->autoloottype) = 1; // Autoloot Activated
			(my->autoloottype) |= (1<<type); // Stores the type
			sprintf(custom_atcmd_output, "Autolooting item type: '%s' {%d}", custom_itemdb_typename(type), type); // Autolooting item type: '%s' {%d}
			clif->message(fd, custom_atcmd_output);
			break;
		case 2:
			if (!((my->autoloottype)&(1<<type))) {
				clif->message(fd,"You're currently not autolooting this item type."); // You're currently not autolooting this item type.
				return -1;
			}
			my->autoloottype &= ~(1<<type);
			sprintf(custom_atcmd_output,"Removed item type: '%s' {%d} from your autoloottype list.", custom_itemdb_typename(type), type); // Removed item type: '%s' {%d} from your autoloottype list.
			clif->message(fd, custom_atcmd_output);
			if (my->autoloottype == ITEM_NONE)
				my->autolootingtype = 0;
			break;
		case 3:
			clif->message(fd,"To add an item type to the list, use @aloottype +<type name or ID>. To remove an item type, use @aloottype -<type name or ID>.");
			clif->message(fd,"Type List: healing = 0, usable = 2, etc = 3, weapon = 4, armor = 5, card = 6, petegg = 7, petarmor = 8, ammo = 10");
			clif->message(fd,"@aloottype reset will clear your autoloottype list.");
			if (my->autoloottype == ITEM_NONE)
				clif->message(fd,"Your autoloottype list is empty.");
			else {
				clif->message(fd,"Item types on your autoloottype list:");
				while (i < IT_MAX) {
					if (my->autoloottype&(1<<i)) {
						sprintf(custom_atcmd_output, "  '%s' {%d}", custom_itemdb_typename(i), i);
						clif->message(fd, custom_atcmd_output);
					}
					i++;
				}
			}
			break;
		case 4:
			my->autoloottype = ITEM_NONE;
			my->autolootingtype = 0;
			clif->message(fd,"Your autoloottype list has been reset.");
			break;
	}
	return true;
}

/* run when server starts */
HPExport void plugin_init (void) {
	iMalloc = GET_SYMBOL("iMalloc");
	atcommand = GET_SYMBOL("atcommand");
	pc = GET_SYMBOL("pc");
	itemdb = GET_SYMBOL("itemdb");
	clif = GET_SYMBOL("clif");
	
	// Add the @command
	if( HPMi->addCommand != NULL ) {
		HPMi->addCommand("autoloottype",ACMD_A(autoloottype));
	}
	
	// Add the hook
	addHookPost("pc->isautolooting",my_pc_isautolooting_post);
}
/* run when server is ready (online) */
HPExport void server_online (void) {
}
/* run when server is shutting down */
HPExport void plugin_final (void) {
}