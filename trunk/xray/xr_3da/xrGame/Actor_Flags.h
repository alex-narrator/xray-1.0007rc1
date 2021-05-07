#pragma once

enum{
		AF_GODMODE			=(1<<0),
		AF_INVISIBLE		=(1<<1),
		AF_ALWAYSRUN		=(1<<2),
		AF_UNLIMITEDAMMO	=(1<<3),
		AF_RUN_BACKWARD		=(1<<4),
		AF_AUTOPICKUP		=(1<<5),
		AF_PSP				=(1<<6),
		AF_AMMO_FROM_BELT	=(1<<7), //патроны с пояса
		AF_QUICK_FROM_BELT	=(1<<8), //наполнение быстрых слотов с пояса
		AF_BULLET_FROM_BARREL =(1<<9), //Пуля/ракета летит из fire_bone в точку куда направлена камера
};

extern Flags32 psActorFlags;

extern BOOL		GodMode	();	

