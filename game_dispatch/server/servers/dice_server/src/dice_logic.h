
#ifndef GAME_DICE_LOGIC_HEAD_FILE
#define GAME_DICE_LOGIC_HEAD_FILE


#include "poker/poker_logic.h"

namespace game_dice
{
//////////////////////////////////////////////////////////////////////////

	enum enDiceType
	{
		CT_ERROR            = 0,	   //��������

		CT_SUM_SMALL        = 1,       //С
		CT_SUM_BIG          = 2,       //��

		CT_POINT_THREE      = 3,       //3��(ʵ���޴�ѹע��)
		CT_POINT_FOUR       = 4,
		CT_POINT_FIVE       = 5,
		CT_POINT_SIX        = 6,
		CT_POINT_SEVEN      = 7,
		CT_POINT_EIGHT      = 8,
		CT_POINT_NINE       = 9,
		CT_POINT_TEN        = 10,
		CT_POINT_ELEVEN     = 11,
		CT_POINT_TWELVE     = 12,
		CT_POINT_THIR_TEEN  = 13,
		CT_POINT_FOUR_TEEN  = 14,
		CT_POINT_FIF_TEEN   = 15,
		CT_POINT_SIX_TEEN   = 16,
		CT_POINT_SEVEN_TEEN = 17,
		CT_POINT_EIGHT_TEEN = 18,       //18��(ʵ���޴�ѹע��)

		CT_ANY_CICLE_DICE   = 19,       //����Χ��
		CT_LIMIT_CICLE_DICE = 20,       //ָ��Χ��

		CT_ONE            = 21,			//һ��1��
		CT_TWO            = 22,			//һ��2��
		CT_THREE          = 23,			//һ��3��
		CT_FOUR           = 24,			//һ��4��
		CT_FIVE           = 25,			//һ��5��
		CT_SIX            = 26,			//һ��6��

		CT_TWICE_ONE      = 27,			//����1��
		CT_TWICE_TWO      = 28,			//����2��
		CT_TWICE_THREE    = 29,			//����3��
		CT_TWICE_FOUR     = 30,			//����4��
		CT_TWICE_FIVE     = 31,			//����5��
		CT_TWICE_SIX      = 32,			//����6��

		CT_TRIPLE_ONE     = 34,			//����1��
		CT_TRIPLE_TWO     = 35,			//����2��
		CT_TRIPLE_THREE   = 36,			//����3��
		CT_TRIPLE_FOUR    = 37,			//����4��
		CT_TRIPLE_FIVE    = 38,			//����5��
		CT_TRIPLE_SIX     = 39,			//����6��
	};

//////////////////////////////////////////////////////////////////////////
#define GAME_PLAYER					4									//��λ����
#define MAX_SCORE_HISTORY           20									//��ʷ����
#define DICE_POINT_COUNT            6                                   //���ӵ���
#define DICE_COUNT                  3                                   //���Ӹ���


#define CONTROL_TRY_TIMES           30

#define AREA_COUNT              	40                                  //ѹע����
#define BET_MONEY_NUMBER			4									//ѹע�������
//////////////////////////////////////////////////////////////////////////
//����
	#define  Multiple_CT_ERROR             	0			//��������

	#define  Multiple_CT_SUM_SMALL         	1			//С
	#define  Multiple_CT_SUM_BIG           	1			//��

	#define  Multiple_CT_POINT_THREE       	0			//3��(ʵ���޴�ѹע��)
	#define  Multiple_CT_POINT_FOUR        	60
	#define  Multiple_CT_POINT_FIVE        	30
	#define  Multiple_CT_POINT_SIX         	17
	#define  Multiple_CT_POINT_SEVEN       	12
	#define  Multiple_CT_POINT_EIGHT       	8
	#define  Multiple_CT_POINT_NINE        	6
	#define  Multiple_CT_POINT_TEN         	6
	#define  Multiple_CT_POINT_ELEVEN      	6
	#define  Multiple_CT_POINT_TWELVE      	6
	#define  Multiple_CT_POINT_THIR_TEEN   	8
	#define  Multiple_CT_POINT_FOUR_TEEN   	12
	#define  Multiple_CT_POINT_FIF_TEEN    	17
	#define  Multiple_CT_POINT_SIX_TEEN    	30
	#define  Multiple_CT_POINT_SEVEN_TEEN  	60
	#define  Multiple_CT_POINT_EIGHT_TEEN  	0			//18��(ʵ���޴�ѹע��)

	#define  Multiple_CT_ANY_CICLE_DICE    	30			//����Χ��
	#define  Multiple_CT_LIMIT_CICLE_DICE  	180			//ָ��Χ��	

	#define  Multiple_CT_ONE             	3			//һ��1��
	#define  Multiple_CT_TWO             	3			//һ��2��
	#define  Multiple_CT_THREE           	3			//һ��3��
	#define  Multiple_CT_FOUR            	3			//һ��4��
	#define  Multiple_CT_FIVE            	3			//һ��5��
	#define  Multiple_CT_SIX             	3			//һ��6��

	#define  Multiple_CT_TWICE_ONE       	8			//����1��
	#define  Multiple_CT_TWICE_TWO       	8			//����2��
	#define  Multiple_CT_TWICE_THREE     	8			//����3��
	#define  Multiple_CT_TWICE_FOUR      	8			//����4��
	#define  Multiple_CT_TWICE_FIVE      	8			//����5��
	#define  Multiple_CT_TWICE_SIX       	8			//����6��

	#define  Multiple_CT_TRIPLE_ONE      	180			//����1��
	#define  Multiple_CT_TRIPLE_TWO      	180			//����2��
	#define  Multiple_CT_TRIPLE_THREE    	180			//����3��
	#define  Multiple_CT_TRIPLE_FOUR     	180			//����4��
	#define  Multiple_CT_TRIPLE_FIVE     	180			//����5��
	#define  Multiple_CT_TRIPLE_SIX      	180			//����6��

//////////////////////////////////////////////////////////////////////////


//��Ϸ�߼�
class CDiceLogic : public CPokerLogic
{
	//��������
public:
	//���캯��
	CDiceLogic();
	//��������
	virtual ~CDiceLogic();

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

	//��������
public:
	static const BYTE m_cbDiceData[DICE_POINT_COUNT];
	BYTE m_nDice[DICE_COUNT];					//��������
	int m_nCountNum[DICE_POINT_COUNT];
	int m_nSumPoint;						   //������
	enDiceType m_enBigSmall;                   //��С


public:
	enDiceType m_enNumber_1;                   //1�����Ӹ���
	enDiceType m_enNumber_2;                   //2�����Ӹ���
	enDiceType m_enNumber_3;                   //3�����Ӹ���
	enDiceType m_enNumber_4;                   //4�����Ӹ���
	enDiceType m_enNumber_5;                   //5�����Ӹ���
	enDiceType m_enNumber_6;                   //6�����Ӹ���

	int m_nCompensateRatio[AREA_COUNT];		   //����

public:
	bool							m_bIsControlDice;
	BYTE							m_cbControlDice[DICE_COUNT];				//��������
	void ResetGameData();

	//�߼�����
public:
	//��������
	void RandDice(BYTE cbDiceBuffer[], BYTE cbPointNumber);
	//ҡ����
	void ShakeRandDice(BYTE cbDiceBuffer[], BYTE cbDiceCount);
	//�������ӽ��
	void ComputeDiceResult();

	//��ȡ������
	enDiceType GetDicePoint(BYTE nDiceArray[]);

	//��ȡ��С
	enDiceType GetBigSmall(BYTE nDiceArray[]);

	//Χ��
	bool IsWaidice();

	//˫����
	bool IsCoupleDice(BYTE cbDiceData[]);

	//��ȡ������ͬ��������
	enDiceType GetThreeSameDice(BYTE nDiceArray[]);

	//��ȡ��ͬ���������Ӹ���
	void CountSameDice(BYTE nDiceArray[]);

	bool CompareHitPrize(int nBetIndex,int &nMultiple);

};

//////////////////////////////////////////////////////////////////////////
};
#endif
