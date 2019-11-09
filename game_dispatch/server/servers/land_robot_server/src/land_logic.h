
#ifndef LAND_LOGIC_HEAD_FILE
#define LAND_LOGIC_HEAD_FILE


#include "svrlib.h"
#include "poker/poker_logic.h"

using namespace std;
using namespace svrlib;

namespace game_land
{

//��Ŀ����
static const int  GAME_LAND_PLAYER  = 3;                                     // ��������Ϸ����
static const int  MAX_LAND_COUNT    = 20;                                    // �����Ŀ

//�߼���Ŀ
static const int NORMAL_COUNT 		= 17;                                    //������Ŀ
static const int DISPATCH_COUNT 	= 51;                                    //�ɷ���Ŀ
static const int GOOD_CARD_COUTN	= 38;									 //������Ŀ

static const int MAX_TYPE_COUNT     = 254;

#define MAX_SIG_CARD_COUNT			16

//�߼�����
enum emLAND_CARD_TYPE {
	CT_ERROR = 0,                                 //��������
	CT_SINGLE,                                    //��������
	CT_DOUBLE,                                    //��������
	CT_THREE,                                     //��������
	CT_SINGLE_LINE,                               //��������
	CT_DOUBLE_LINE,                               //��������
	CT_THREE_LINE,                                //��������
	CT_THREE_TAKE_ONE,                            //����һ��
	CT_THREE_TAKE_TWO,                            //����һ��
	CT_FOUR_TAKE_ONE,                             //�Ĵ�����
	CT_FOUR_TAKE_TWO,                             //�Ĵ�����
	CT_BOMB_CARD,                                 //ը������
	CT_MISSILE_CARD,                              //�������
};

//��������
#define ST_ORDER                    1                                    //��С����
#define ST_COUNT                    2                                    //��Ŀ����
#define ST_CUSTOM                   3                                    //�Զ�����

//////////////////////////////////////////////////////////////////////////////////

//�����ṹ
struct tagAnalyseResult {
	BYTE cbBlockCount[4];                    //�˿���Ŀ
	BYTE cbCardData[4][MAX_LAND_COUNT];        //�˿�����
};

//���ƽ��
	struct tagOutCardResult {
		BYTE cbCardCount;                        //�˿���Ŀ
		BYTE cbResultCard[MAX_LAND_COUNT];        //����˿�
	};

//�ֲ���Ϣ
	struct tagDistributing {
		BYTE cbCardCount;                        //�˿���Ŀ
		BYTE cbDistributing[15][6];                //�ֲ���Ϣ
	};

//�������
	struct tagSearchCardResult {
		BYTE cbSearchCount;                                    //�����Ŀ
		BYTE cbCardCount[MAX_LAND_COUNT];                      //�˿���Ŀ
		BYTE cbResultCard[MAX_LAND_COUNT][MAX_LAND_COUNT];     //����˿�
	};

    struct tagOutCardTypeResult 
    {
    	BYTE							cbCardType;							//�˿�����
    	BYTE							cbCardTypeCount;					//������Ŀ
    	BYTE							cbEachHandCardCount[MAX_TYPE_COUNT];//ÿ�ָ���
    	BYTE							cbCardData[MAX_TYPE_COUNT][MAX_LAND_COUNT];//�˿�����
    };

//////////////////////////////////////////////////////////////////////////////////

//��Ϸ�߼���
	class CLandLogic : public CPokerLogic
	{
		//��������
	public:
		static const BYTE m_cbCardData[FULL_POKER_COUNT];            //�˿�����
	    //AI����
    public:
    	//static const BYTE				m_cbGoodcardData[GOOD_CARD_COUTN];	                                    //��������
    	BYTE							m_cbAllCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];                      //�����˿�
    	BYTE							m_cbLandScoreCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];	            //�����˿�
    	BYTE							m_cbUserCardCount[GAME_LAND_PLAYER];		                            //�˿���Ŀ
    	WORD							m_wBankerUser;						                                    //�������

		//��������
	public:
		//���캯��
		CLandLogic();

		//��������
		virtual ~CLandLogic();

		//���ͺ���
	public:
		//��ȡ����
		BYTE GetCardType(const BYTE cbCardData[], BYTE cbCardCount);
		//���ƺ���
	public:
		//�����˿�
		void RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount);
		void ShuffleCard(BYTE cbCardData[], BYTE cbCardCount);

		//�����˿�
		void SortCardList(BYTE cbCardData[], BYTE cbCardCount, BYTE cbSortType);

		//ɾ���˿�
		bool RemoveCardList(const BYTE cbRemoveCard[], BYTE cbRemoveCount, BYTE cbCardData[], BYTE cbCardCount);

		//�߼�����
	public:
		//�߼���ֵ
		BYTE GetCardLogicValue(BYTE cbCardData);

		//�Ա��˿�
		bool CompareCard(const BYTE cbFirstCard[], const BYTE cbNextCard[], BYTE cbFirstCount, BYTE cbNextCount);
		//��������
		bool SearchOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, const BYTE cbTurnCardData[], BYTE cbTurnCardCount, tagOutCardResult & OutCardResult);
		//�ڲ�����
	public:
		//�����˿�
		void AnalysebCardData(const BYTE cbCardData[], BYTE cbCardCount, tagAnalyseResult &AnalyseResult);

		//�����ֲ�
		void AnalysebDistributing(const BYTE cbCardData[], BYTE cbCardCount, tagDistributing &Distributing);

    	//AI����
    public:
    	//�����˿�
    	void SetUserCard(WORD wChairID, BYTE cbCardData[], BYTE cbCardCount) ;
    	//���õ���
    	void SetBackCard(WORD wChairID, BYTE cbBackCardData[], BYTE cbCardCount) ;
    	//����ׯ��
    	void SetBanker(WORD wBanker) ;
    	//�����˿�
    	void SetLandScoreCardData(WORD wChairID,BYTE cbCardData[], BYTE cbCardCount) ;
    	//ɾ���˿�
    	void RemoveUserCardData(WORD wChairID, BYTE cbRemoveCardData[], BYTE cbRemoveCardCount) ;

    	//��������
    public:
    	//����㷨
    	void Combination(BYTE cbCombineCardData[], BYTE cbResComLen,  BYTE cbResultCardData[254][5], BYTE &cbResCardLen,BYTE cbSrcCardData[] , BYTE cbCombineLen1, BYTE cbSrcLen, const BYTE cbCombineLen2);
    	//�����㷨
    	void Permutation(BYTE *list, int m, int n, BYTE result[][4], BYTE &len) ;
    	//����ը��
    	void GetAllBomCard(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE cbBomCardData[], BYTE &cbBomCardCount);
    	//����˳��
    	void GetAllLineCard(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE cbLineCardData[], BYTE &cbLineCardCount);
    	//��������
    	void GetAllThreeCard(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE cbThreeCardData[], BYTE &cbThreeCardCount);
    	//��������
    	void GetAllDoubleCard(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE cbDoubleCardData[], BYTE &cbDoubleCardCount);
    	//��������
    	void GetAllSingleCard(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE cbSingleCardData[], BYTE &cbSingleCardCount);

    	//��Ҫ����
    public:
    	//�������ͣ�����Ƶ��ã�
    	void AnalyseOutCardType(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE const cbTurnCardData[], BYTE const cbTurnCardCount, tagOutCardTypeResult CardTypeResult[12+1]);
    	//�������ƣ��ȳ��Ƶ��ã�
    	void AnalyseOutCardType(BYTE const cbHandCardData[], BYTE const cbHandCardCount, tagOutCardTypeResult CardTypeResult[12+1]);
    	//���Ƹ���
    	BYTE AnalyseSinleCardCount(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE const cbWantOutCardData[], BYTE const cbWantOutCardCount, BYTE cbSingleCardData[]=NULL);

    	//���ƺ���
    public:
    	//�������ƣ��ȳ��ƣ�
    	void BankerOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, tagOutCardResult & OutCardResult) ;
    	//�������ƣ�����ƣ�
    	void BankerOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, WORD wOutCardUser, const BYTE cbTurnCardData[], BYTE cbTurnCardCount, tagOutCardResult & OutCardResult) ;
    	//�����ϼң��ȳ��ƣ�
    	void UpsideOfBankerOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, WORD wMeChairID,tagOutCardResult & OutCardResult) ;
    	//�����ϼң�����ƣ�
    	void UpsideOfBankerOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, WORD wOutCardUser,  const BYTE cbTurnCardData[], BYTE cbTurnCardCount, tagOutCardResult & OutCardResult) ;
    	//�����¼ң��ȳ��ƣ�
    	void UndersideOfBankerOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, WORD wMeChairID,tagOutCardResult & OutCardResult) ;
    	//�����¼ң�����ƣ�
    	void UndersideOfBankerOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, WORD wOutCardUser, const BYTE cbTurnCardData[], BYTE cbTurnCardCount, tagOutCardResult & OutCardResult) ;
    	//��������
    	bool SearchOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, const BYTE cbTurnCardData[], BYTE cbTurnCardCount, WORD wOutCardUser, WORD wMeChairID, tagOutCardResult & OutCardResult);

    	//�зֺ���
    public:
    	//�з��ж�
    	BYTE LandScore(WORD wMeChairID, BYTE cbCurrentLandScore) ;

		BYTE GetCardIndex(BYTE cbCardData);





	};
};

#endif

