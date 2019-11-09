//
// Created by toney on 16/4/12.
//

#ifndef SERVER_SANGONG_LOGIC_H
#define SERVER_SANGONG_LOGIC_H

#include "poker_logic.h"

namespace game_sangong
{
//�궨��

static const int GAME_PLAYER	    =			6;									//��Ϸ����

//��Ŀ����
static const int FULL_COUNT		=			    52;									//ȫ����Ŀ
static const int MAX_COUNT		=				3;									//�����Ŀ

//�˿�����
enum emSANGONG_CARD_TYPE
{
	//�˿�����
 	OX_VALUE0					 = 0,									//�������
	OX_VALUE1,
	OX_VALUE2,
	OX_VALUE3,
	OX_VALUE4,
	OX_VALUE5,
	OX_VALUE6,
	OX_VALUE7,
	OX_VALUE8,
	OX_VALUE9,
 	OX_THREE_KING0,														//����������
	OX_THREE_KING1,														//С����
	OX_THREE_KING2,														//������
};

//��Ϸ�߼���
class CSangongLogic : public CPokerLogic
{
	//��������
private:
	static BYTE						m_cbCardListData[52];			//�˿˶���    
	//��������
public:
	//���캯��
	CSangongLogic();
	//��������
	virtual ~CSangongLogic();

	//���ͺ���
public:
	//��ȡ����
	BYTE GetCardType(BYTE cbCardData[], BYTE cbCardCount);
	//��ȡ����
	BYTE GetTimes(BYTE cbCardData[], BYTE cbCardCount);
	//��ȡţţ
	bool GetOxCard(BYTE cbCardData[], BYTE cbCardCount);
	//��ȡ����
	bool IsIntValue(BYTE cbCardData[], BYTE cbCardCount);

	//���ƺ���
public:
	//�����˿�
	void SortCardList(BYTE cbCardData[], BYTE cbCardCount);
	//�����˿�
	void RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount);

	//���ܺ���
public:
	//�߼���ֵ
	BYTE GetCardLogicValue(BYTE cbCardData);
	//�Ա��˿�
	bool CompareCard(BYTE cbFirstData[], BYTE cbNextData[], BYTE cbCardCount,BYTE& multiple);

};


};


#endif //SERVER_SANGONG_LOGIC_H

