//===== Hercules Plugin ======================================
//= @gameboy
//===== By: ==================================================
//= LaVit
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Automatically Searches and Attacks nearby monsters.
//===== Changelog: ===========================================
//= v1.0 - Initial Conversion
//============================================================

#include "common/hercules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/HPMi.h"
#include "common/timer.h"
#include "common/nullpo.h"

#include "map/atcommand.h"
#include "map/map.h"
#include "map/pc.h"

#include "map/itemdb.h"
#include "map/storage.h"

#include "map/script.h"
#include "map/unit.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

#define OPTION_AUTOATTACK 0x40000000

HPExport struct hplugin_info pinfo = {
	"@gameboy",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};

static int buildin_autoattack_sub(struct block_list *bl, va_list ap)
{
	int *target_id = va_arg(ap,int *);
	if (*target_id != 0)
		return 1;

	*target_id = bl->id;
	return 1;
}

int check_and_consume_fuel(struct map_session_data *sd) {
    int fuel_id = 501;  // ID da pilha
    int amount = 0;
    int i = 0;
    int id = 0;

    for (i = 0; i < sd->status.inventorySize; i++ ) {
        if( sd->status.inventory[i].nameid == fuel_id ) {
            amount = sd->status.inventory[i].amount;
            id = i;
        }
    }

    if (amount > 0) {
        pc->delitem(sd, id, 1, 0, DELITEM_NORMAL, LOG_TYPE_INV_INVALID);
        clif->message(sd->fd, "Você consumiu uma pilha.");
        return 1;
    } else {
        clif->message(sd->fd, "Você não tem pilhas suficiente para ligar o gameboy.");
        return 0;
    }
}

void autoattack_fuel(struct map_session_data* sd)
{
    // Verificar e consome uma pilha a cada 1 minuto
    if (!check_and_consume_fuel(sd)) {
        sd->sc.option &= ~OPTION_AUTOATTACK;
        unit->stop_attack(&sd->bl);
        clif->changelook(&sd->bl, LOOK_HEAD_TOP, 0);  // Remove ícone
        clif->message(sd->fd, "Gameboy DESLIGADO por falta de pilha.");
    	// clif->sitting(&sd->bl);
        return;
    }
}

void autoattack_motion(struct map_session_data* sd)
{
    int i, target_id;

	for (i = 0; i <= 9; ++i) {
		target_id = 0;
		map->foreachinarea(buildin_autoattack_sub, sd->bl.m, sd->bl.x-i, sd->bl.y-i, sd->bl.x+i, sd->bl.y+i, BL_MOB, &target_id);
		if (target_id) {
			unit->attack(&sd->bl, target_id, 1);
			break;
		}
	}
	if (target_id == 0)
		unit->walk_toxy(&sd->bl, sd->bl.x+(rand()%2==0?-1:1)*(rand()%10), sd->bl.y+(rand()%2==0?-1:1)*(rand()%10),0);
	return;
}

int autoattack_timer(int tid, int64 tick, int id, intptr_t data)
{
	struct map_session_data *sd = NULL;

	sd = map->id2sd(id);
	if(sd == NULL)
		return 0;
	if (sd->sc.option & OPTION_AUTOATTACK) {
		autoattack_motion(sd);
		timer->add(timer->gettick()+2000, autoattack_timer, sd->bl.id, 0);
	}
	return 0;
}

int autoattack_timer_fuel(int tid, int64 tick, int id, intptr_t data)
{
	struct map_session_data *sd = NULL;

	sd = map->id2sd(id);
	if(sd == NULL)
		return 0;
	if (sd->sc.option & OPTION_AUTOATTACK) {
		autoattack_fuel(sd);
		timer->add(timer->gettick()+60000, autoattack_timer_fuel, sd->bl.id, 0);
	}
	return 0;
}

ACMD(autoattack)
{
	if (sd->sc.option & OPTION_AUTOATTACK) {
		sd->sc.option &= ~OPTION_AUTOATTACK;
		unit->stop_attack(&sd->bl);
		clif->message(fd, "Gameboy DESLIGADO");

        // Remove o ícone ao desativar
        clif->changelook(&sd->bl, LOOK_HEAD_TOP, 0);
	} else {
        if (check_and_consume_fuel(sd)) {
            sd->sc.option |= OPTION_AUTOATTACK;
            timer->add(timer->gettick()+2000, autoattack_timer, sd->bl.id, 0);
            timer->add(timer->gettick()+60000, autoattack_timer_fuel, sd->bl.id, 0);
            clif->message(fd, "Gameboy LIGADO");

            // Adiciona um ícone ao ativar
            clif->changelook(&sd->bl, LOOK_HEAD_TOP, 1552);
        } else {
            clif->message(fd, "Você não tem pilhas suficiente para iniciar o Gameboy.");
        }
	}
	clif->changeoption(&sd->bl);
	return true;
}

HPExport void plugin_init(void)
{
	addAtcommand("gameboy", autoattack);
}


HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by LaVit. Version '%s'\n", pinfo.name, pinfo.version);
}