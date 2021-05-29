#pragma once

enum{
		AF_GODMODE			 =(1<<0),
		AF_INVISIBLE		 =(1<<1),
		AF_ALWAYSRUN		 =(1<<2),
		AF_UNLIMITEDAMMO	 =(1<<3),
		AF_RUN_BACKWARD		 =(1<<4),
		AF_AUTOPICKUP		 =(1<<5),
		AF_PSP				 =(1<<6),
		AF_AMMO_FROM_BELT	 =(1<<7), //������� � �����
		AF_QUICK_FROM_BELT	 =(1<<8), //���������� ������� ������ � �����
		AF_NO_AUTO_RELOAD    =(1<<9), //������ ��������������� ������
		AF_HARD_INV_ACCESS   =(1<<10), //����������� ������ � ���������, ���, ������� ������
		AF_CORPSES_COLLISION =(1<<11), //�������� ������
};

extern Flags32 psActorFlags;

extern BOOL		GodMode	();	

