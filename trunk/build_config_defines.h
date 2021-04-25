#pragma once
// ==================================== Правки от alpet ======================================= 
#define HLAMP_AFFECT_IMMUNITIES			// включает обработку иммунитетов для висячих ламп (CHangingLamp)
// #define LUAICP_COMPAT				// включает совместимость с перехватчиком Lua (luaicp.dll). Задавать надо в свойствах 
#define SCRIPT_ICONS_CONTROL		    // включает экспорт функции setup_game_icon в Lua
#define SCRIPT_EZI_CONTROL				// включает экспорт класса CEffectorZoomInertion для управления движением прицела 
#define SPAWN_ANTIFREEZE				// распределяет массовый спавн объектов по кадрам, чтобы исключить продолжительные фризы 
#define ECO_RENDER						// добавляет небольшую задержку между рендерингом кадров, чтобы зря не насиловать видеокарту при больших FPS
// #define MT_OPT						// задает выделенные ядра ЦП для второго потока
//#define AF_SHOW_DYNAMIC_PARAMS		// для артефактов будут выводиться действующие свойства, а не из файла конфигурации (т.е. без учета отношения к хар-кам актора - прим. xer-urg)
#define SCRIPT_VARS_STORAGE				// включает хранилище скриптовых переменных в сейвы
//#define RUCK_FLAG_PREFERRED           // позволяет всем предметам реагировать на флаг FRuckDefault. Если флаг установлен, хабар будет подбираться в рюкзак, а не в слот по умолчанию.
#define SHORT_LIVED_ANOMS               // поддержка временных аномалий, по умолчанию работающих в мультиплеере.
//#define OBJECTS_RADIOACTIVE           // активирует потенциальную радиоактивность всех объектов, а не только артефактов.

// ==================================== Правки от K.D. =======================================
#define KD_DETAIL_RADIUS				// alpet: включает правку дистанции отрисовки травы и плотности. Может влиять на производительность для слабых систем.

// ==================================== Правки от Real Wolf ======================================= 
#define INV_RUCK_UNLIMITED_FIX		// Real Wolf: позволяет безболезненно использовать атрибут unlimited в теге dragdrop_bag для создания лимитного инвентаря
#define INV_NEW_SLOTS_SYSTEM			// Real Wolf, Red Virus: включает слоты.
#define SUN_DIR_NOT_DEBUG			    // Real Wolf: отключение вывода в лог информации вида CurrentEnv.sun_dir...
#define ARTEFACTS_FROM_RUCK			    // Real Wolf: артефакты будут работать из всего инвентаря (слотов, пояса, рюкзака).
#define KNIFE_SPRINT_FIX				// Real Wolf: остановка спринта при ударе2.
#define KNIFE_SPRINT_MOTION				// Real Wolf: вопроизведение анимации бега для ножа.
#define MISSILE_THREAT_FIX				// Real Wolf: остановка спринта при броске гранаты, болта и т.д.
#define GRENADE_FROM_BELT				// Real Wolf: гранаты с пояса.
#define LOCK_RELOAD_IN_SPRINT			// Real Wolf: блокировка перезарядки по время спринта.
#define HIDE_WEAPON_IN_CAR				// Real Wolf: прятать все оружие в машине.
#define BM16_ANIMS_FIX					// Real Wolf: расширение анимаций для BM16.
#define INV_OUTFIT_FULL_ICON_HIDE		// Real Wolf: иконка игрока в костюме заменена на иконку костюма.
//#define UI_LOCK_PDA_WITHOUT_PDA_IN_SLOT // Real Wolf: блокировать работу ПДА, если самого предмета нету в слоте. --поведение по умолчанию
#define CAR_SAVE_FUEL					// Real Wolf: сохранение текущего объема топлива, максимального объема и потребления машины.
//#define R1_EXCLUDE					// Real Wolf: отключает первый рендер, оставляя только полное динамическое освещение.
#define EAT_PORTIONS_INFLUENCE          // Real Wolf: Уменьшаем вес и цену после использования.

// ==================================== Правки от Red Virus ======================================= 
#define WPN_BOBBING						// Red Virus: bobbing effect from lost alpha
#define INV_COLORIZE_AMMO				// Red Virus: colorize ammo from lost alpha
//
#ifdef INV_NEW_SLOTS_SYSTEM
	#define INV_NO_ACTIVATE_APPARATUS_SLOT  // Red Virus: убирает невозможность сменить оружие клавишами при активных слотах:  фонарика, детектора и тд
	#define INV_MOVE_ITM_INTO_QUICK_SLOTS	// Red Virus: позволяет менять местами предметы в быстрых слотах
	#define INV_QUICK_SLOT_PANEL		    // Red Virus: панель быстрых слотов на главном экране
#endif

// ==================================== Правки от Karlan ======================================= 
//#define AMMO_FROM_BELT					// Karlan: Патроны используются только с пояса --вынесено в опции

// ==================================== Правки от xer-urg =======================================
//#define MY_DEBUG                      //отладочные сообщения
//
#ifdef INV_NEW_SLOTS_SYSTEM
       #define SHOW_ARTEFACT_SLOT       //отображение слота артефакта в инвентаре
     //  #define QUICK_MEDICINE           //быстрые слоты назначаются аптечке, бинту и антираду по умолчанию (а не любой еде, как в оригинальном репозитории)
     //  #define QUICK_SLOT_POCKET_LOGIC  //после использования предмета в быстром слоте следующий аналогичный предмет не встаёт в слот из инвентаря
#endif
//
#ifdef GRENADE_FROM_BELT
       #define SHOW_GRENADE_SLOT       //отображение слота гранаты в инвентаре
#endif
//
#define DISABLE_MAX_WALK_WEIGHT        //отключает учет параметра max_walk_weight, в т.ч. и обездвиживание при превышении
//#define NO_DBCLICK_USE                 //отключает использование предметов в инвентаре по даблклику (поедание еды, использование медицины, установка обвесов на оружие)
#define ACTOR_PARAMS_DEPENDECY         //скорость ходьбы и высота прыжка зависят от здоровья и веса перегруза
#define AF_JUMP_WALK                   //артефакт влияют на высоту прыжка и скорость ходьбы
#define SATIETY_SET_MAX_POWER          //сытость напрямую влияет на максимальную выносливость
#define SIMPLE_ZOOM_SETTINGS           //кратность оптики задаётся через степень увеличения а не fov, прицеливание без оптики не зуммирует (если не указан ironsight_zoom_factor)
#define ITEMS_ON_AF_PANEL              //на худовой панели артефактов отображаются любые предметы, помещенные на пояс
#define CARBODY_COLORIZE_ITEM          //подсветка предметов в слотах и на поясе в окне обыска ящиков/трупов
#define RADIATION_PARAMS_DEPENDECY     //задаёт пороговые значения радиации больше которых не восстанавливается здоровье, выносливость и снижается макимальный уровень здоровья (хит радиации здоровью лучше отключить)
//#define NO_RAD_UI_WITHOUT_DETECTOR_IN_SLOT  //не отображать прогрессбар в инвентаре и иконку радиации на худе если в слоте нет детектора -- поведение по умолчанию