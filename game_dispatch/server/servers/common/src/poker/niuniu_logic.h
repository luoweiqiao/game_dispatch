
#ifndef GAME_NIUNIU_LOGIC_HEAD_FILE
#define GAME_NIUNIU_LOGIC_HEAD_FILE


#include "poker_logic.h"

namespace game_niuniu
{
//////////////////////////////////////////////////////////////////////////
//����
	struct tagNiuMultiple {
		int32	niu1;
		int32	niu2;
		int32	niu3;
		int32	niu4;
		int32	niu5;
		int32	niu6;
		int32	niu7;
		int32	niu8;
		int32	niu9;
		int32	niuniu;
		int32	big5niu;
		int32	small5niu;
		int32   bomebome;
		tagNiuMultiple() {
			Init();
		}
		void Init() {
			niu1 = 0;
			niu2 = 0;
			niu3 = 0;
			niu4 = 0;
			niu5 = 0;
			niu6 = 0;
			niu7 = 0;
			niu8 = 0;
			niu9 = 0;
			niuniu = 0;
			big5niu = 0;
			small5niu = 0;
			bomebome = 0;
		}
	};
enum emCardType
{
	CT_ERROR			=		0,								//��������
	CT_POINT			=		1,								//��������
	CT_SPECIAL_NIU1		=		2,								//ţһ
	CT_SPECIAL_NIU2		=		3,								//ţ��
	CT_SPECIAL_NIU3		=		4,								//ţ��
	CT_SPECIAL_NIU4		=		5,								//ţ��
	CT_SPECIAL_NIU5		=		6,								//ţ��
	CT_SPECIAL_NIU6		=		7,								//ţ��
	CT_SPECIAL_NIU7		=		8,								//ţ��
	CT_SPECIAL_NIU8		=		9,								//ţ��
	CT_SPECIAL_NIU9		=	    10,								//ţ��
	CT_SPECIAL_NIUNIU	=		11,								//ţţ
	CT_SPECIAL_NIUNIUDW	=		12,								//����ţ(����ȫ��JQK)
    CT_SPECIAL_NIUNIUXW	=		13,								//С��ţ(����A234)
	CT_SPECIAL_BOMEBOME	=		14								//ը��
};

//��������
#define	ST_VALUE					1									//��ֵ����
#define	ST_NEW					    2									//��ֵ����
#define	ST_LOGIC					3									//�߼�����

//�˿���Ŀ
//#define CARD_COUNT					52									//�˿���Ŀ
static const int CARD_COUNT 	 = 52;
static const int NIUNIU_CARD_NUM = 5;

//��Ϸ�߼�
class CNiuNiuLogic : public CPokerLogic
{
	//��������
public:
	static const BYTE				m_cbCardListData[CARD_COUNT];	//�˿˶���

	//��������
public:
	//���캯��
	CNiuNiuLogic();
	//��������
	virtual ~CNiuNiuLogic();

	//���ͺ���
public:
	//��ȡ��ֵ
	BYTE GetCardValue(BYTE cbCardData) 
	{ 
		return cbCardData&MASK_VALUE; 
	}
	//��ȡ��ɫ
	BYTE GetCardColor(BYTE cbCardData)
	{
		return (cbCardData&MASK_COLOR)>>4;
	}

	//���ƺ���
public:
	//�����˿�
	void RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount);
	//�����˿�
	void RandCardListEx(BYTE cbCardBuffer[], BYTE cbBufferCount);
        
	//�����˿�
	void SortCardList(BYTE cbCardData[], BYTE cbCardCount, BYTE cbSortType);

	int  RetType(int itype);

	//�߼�����
public:
	//��ȡ�Ƶ�
	BYTE GetCardListPip(const BYTE cbCardData[], BYTE cbCardCount);
	//��ȡ����
	BYTE GetCardType(const BYTE cbCardData[], BYTE cbCardCount,BYTE *bcOutCadData = NULL);
    
	//��С�Ƚ�
	int  CompareCard(const BYTE cbFirstCardData[], BYTE cbFirstCardCount, const BYTE cbNextCardData[], BYTE cbNextCardCount, BYTE &Multiple, BYTE multipleType = 0, tagNiuMultiple * ptagNiuMultiple = NULL);
	//�߼���С
	BYTE GetCardLogicValue(BYTE cbCardData);

	BYTE GetCardNewValue(BYTE cbCardData);

	bool GetSubDataCard(BYTE cbSubCardData[][NIUNIU_CARD_NUM], vector<BYTE> & vecRemainCardData);

	bool IsValidCard(BYTE cbCardData);
};

//////////////////////////////////////////////////////////////////////////
};
#endif
