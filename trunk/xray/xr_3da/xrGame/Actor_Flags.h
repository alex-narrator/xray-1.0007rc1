#pragma once

enum{
		AF_GODMODE			  =(1<<0),
		AF_INVISIBLE		  =(1<<1),
		AF_ALWAYSRUN		  =(1<<2),
		AF_UNLIMITEDAMMO	  =(1<<3),
		AF_RUN_BACKWARD		  =(1<<4),
		AF_AUTOPICKUP		  =(1<<5),
		AF_PSP				  =(1<<6),
		AF_AMMO_FROM_BELT	  =(1<<7),  //патроны с пояса
		AF_QUICK_FROM_BELT	  =(1<<8),  //наполнение быстрых слотов с пояса
		AF_NO_AUTO_RELOAD     =(1<<9),  //запрет автоперезарядки оружия
		AF_HARD_INV_ACCESS    =(1<<10), //усложненный доступ к инвентарю, ПДА, быстрым слотам, обыску трупов и ящиков
		AF_ARTEFACTS_FROM_ALL =(1<<11), //артефакты работают из всего инвентаря
		AF_SMOOTH_OVERWEIGHT  =(1<<12), //плавный перегруз без обездвиживания
};

extern Flags32 psActorFlags;

extern BOOL		GodMode	();	

