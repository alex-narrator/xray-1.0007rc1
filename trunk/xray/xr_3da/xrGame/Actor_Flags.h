#pragma once

enum{
		AF_GODMODE			         =(1<<0),
		AF_INVISIBLE		         =(1<<1),
		AF_ALWAYSRUN		         =(1<<2),
		AF_UNLIMITEDAMMO	         =(1<<3),
		AF_RUN_BACKWARD		         =(1<<4),
		AF_AUTOPICKUP		         =(1<<5),
		AF_PSP				         =(1<<6),
		AF_AMMO_FROM_BELT	         =(1<<7),  //патроны с пояса
		AF_QUICK_FROM_BELT	         =(1<<8),  //наполнение быстрых слотов с пояса
		AF_NO_AUTO_RELOAD            =(1<<9),  //запрет автоперезарядки оружия
		AF_ARTEFACTS_FROM_ALL        =(1<<10), //артефакты работают из всего инвентаря
		AF_SMOOTH_OVERWEIGHT         =(1<<11), //плавный перегруз без обездвиживания
		AF_CONDITION_INTERDEPENDENCE =(1<<12), //взаимозависимость параметров здоровья ГГ
		AF_WPN_ACTIONS_RESET_SPRINT  =(1<<13), //перезарядка/смена типа патрона/бросок гранаты/болта/удар ножом сбрасывают спринт
		AF_ARTEFACT_DETECTOR_CHECK   =(1<<14), //свойства артефактов отображаются после проверки детектором
		AF_PICKUP_TARGET_ONLY        =(1<<15), //можно подобрать только те предметы на которые непосредственно смотрит прицел
		AF_PAUSE_AFTER_LOADING       =(1<<16), //пауза после загрузки сохранения
		//AF_AMMO_BOX_AS_MAGAZINE	     =(1<<17), //перезарядка оружия кол-вом патронов в пачке
};

extern Flags32 psActorFlags;

extern BOOL		GodMode	();	

//освобождение рук для взаимодействия с предметами
enum EFreeHandsMode
{
	eFreeHandsOff,		//отключено
	eFreeHandsAuto,		//автоосвобождение
	eFreeHandsManual	//освобождать вручную
};
extern EFreeHandsMode	g_eFreeHands; //освобождение рук для взаимодействия с предметами: 0 - отключено, 1 - автоматически, 2 - вручную

