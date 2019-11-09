#ifndef GAME_FRUIT_LOGIC_HEAD_FILE
#define GAME_FRUIT_LOGIC_HEAD_FILE
#include "svrlib.h"

using namespace std;
#define PRO_DENO_100     100	
#define MAXROW                       3            //�������	
#define MAXCOLUMN                    5            //��Ŀ����
#define MAXWEIGHT                    11           //Ȩ�ظ���


#define	PIC_SEVEN                    11          //����ͼƬ		7
#define	PIC_WILD                     9           //����ͼƬ		�ٴ�
#define	PIC_BONUS                    10           //����ͼƬ	���ת��

#define	MAX_LINE                     9             //�������
//#define	MAX_WHEELS                   100000        //������ͼ���ĸ���
#define	MAX_WHEELS                   312        //������ͼ���ĸ���
#define MAX_RATIO                    100000000      //��󾫶�

//�ߵı��
typedef struct
{
	uint32				lineID; ////��id
	uint32				pos[MAXCOLUMN]; //�߶�Ӧ�ĵ�
}LinePosConfig;
typedef vector<LinePosConfig> ServerLinePosVecType;

//ͼƬ��Ӧ������
typedef struct
{
	uint32				picId;		           //ͼƬid
	uint32				multiple[MAXCOLUMN];	//����
}PicRateConfig;
typedef vector<PicRateConfig> PicRateConfigVecType;

//���߽����¼
typedef struct
{
	uint32 	lineNo;  //�ߺ�
	uint32	picID;	//ͼƬID
	uint32 	linkNum; //��������
}picLinkinfo;
typedef vector<picLinkinfo> picLinkVecType;

//ˮ����ͼ���ϵı��
typedef struct
{
	uint32			id;          //���
	uint32			pic[MAXROW][MAXCOLUMN];     //��Ӧ�� ����
	uint32          LineMultiple[MAX_LINE];     //1-9��������
	uint32          LineBonus[MAX_LINE];        //1-9����Ѵ���
	uint32          SysRatio[MAX_LINE][5];      //9��ϵͳ����1-5����
	uint32          UserRatio[MAX_LINE][2];     //9�߸��˷���1-2����
	uint32          TotalSysRatio[MAX_LINE][5];      //9��ϵͳ����1-5���ں�
	uint32          TotalUserRatio[MAX_LINE][2];     //9�߸��˷���1-2���ں�
}WheelInfo;
typedef vector<WheelInfo> ServerWheelInfoType;

//ˮ����ͼ���ϵı��
typedef struct
{
	uint32 linenum;//�ߺ�
	uint32 index;  //������
	uint32 compRate;   //���ҵķ������� 
}compareDef;

//������
//1   4   7   10   13
//2   5   8   11   14
//3   6   9   12   15

//[2, 5, 8, 11, 14],
//[1, 4, 7, 10, 13],
//[3, 6, 9, 12, 15],
//[1, 5, 9, 11, 13],
//[3, 5, 7, 11, 15],
//[1, 4, 8, 12, 15],
//[3, 6, 8, 10, 13],
//[2, 6, 8, 10, 14],
//[2, 4, 8, 12, 14]


//[20, 43, 43, 43, 72, 38, 28, 60, 20, 10, 0],
//[20, 43, 43, 43, 72, 38, 28, 50, 50, 20, 0], 
//[80, 43, 43, 43, 72, 38, 28, 40, 50, 20, 0], 
//[80, 43, 88, 58, 72, 38, 28, 25, 60, 12, 0], 
//[80, 43, 88, 58, 72, 38, 28, 25, 10, 13, 0]


namespace game_slot
{
	bool compareBySysKey(const WheelInfo & info, const compareDef & defRate);
	bool compareByUseKey(const WheelInfo & info, const compareDef & defRate);
	//��Ϸ�߼�
	class CSlotLogic
	{
		//��������
	public:
		//���캯��
		CSlotLogic();
		//��������
		virtual ~CSlotLogic();
		void Init();
		void MakeWheels();
		void MakeNoRateWheels();
		//��������
	public:
		//���������ĸ���
		uint64 GetLineMultiple(uint32 GamePic[MAXCOLUMN], picLinkinfo &picLink);
		uint32 m_GamePic[MAXROW][MAXCOLUMN];//����3*5

		bool GetRandPicArray(uint32 GamePic[][MAXCOLUMN]);

		//��ȡϵͳ��������ͼƬ����
		bool GetSysPicArray(uint32 randRate, uint32 linenum, uint32 ReturnIndex, uint32 GamePic[]);
		//��ȡ���˷�������ͼƬ����
		bool GetUserPicArray(uint32 randRate, uint32 linenum, uint32 ReturnIndex, uint32 GamePic[]);
		//����Ȩ���㷨,��ȡ�õ��ڼ��������
		uint32 GetWeightIndex(uint32 num);
		//���Ը���Ȩ���㷨,��ȡ�õ��ڼ��������
		uint32 GetNoRateWeightIndex(uint32 posone, uint32 postwo, uint32 posthree);

		uint32 GetFreeSpinWeightIndex(uint32 num);
		uint32 GetMultipleSpinWeightIndex(uint32 num, uint32 nline, int64 lMinWinMultiple, int64 lMaxWinMultiple);
		uint32 GetMultipleRandIndex(uint32 num, uint32 nline, int64 lMinWinMultiple, int64 lMaxWinMultiple);
		bool GetMultiplePicArray(uint32 nline, int64 lMinWinMultiple, int64 lMaxWinMultiple, uint32 GamePic[][MAXCOLUMN]);


		void   testMakeWheels();
		void	GetJackpotScoreByLine(uint32 line, uint32 mGamePic[][MAXCOLUMN]);

		void GetJackpotScoreOne(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreTwo(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreThree(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreFour(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreFive(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreSix(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreSeven(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreEight(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreNine(uint32 mGamePic[][MAXCOLUMN]);


		uint32 GetBananaWeightIndex(uint32 line);
		//��ʼ������
		void SetSlotFeeRate(int64 nRate) { m_FeeRate = nRate; }
		void SetSlotJackpotRate(int64 nRate) { m_JackpotRate = nRate; }
		void SetSlotUserBet(uint64 bet) { m_UserBet = bet; }
		void SetWeightPicCfg(uint32 weightPicCfg[]) { memcpy(m_WeightPicCfg, weightPicCfg, 220); }
		void SetBonusFreeTimes(uint32 BonusFreeTimes[]) { memcpy(m_BonusFreeTimes, BonusFreeTimes,  MAXCOLUMN*4); }
		void SetReturnUserRate(int32 ReturnUserRate[]) { memcpy(m_ReturnUserRate, ReturnUserRate, 2*4); }
		void SetReturnSysRate(int32 ReturnSysRate[])   { memcpy(m_ReturnSysRate, ReturnSysRate, 5*4); }
		void SetServerLinePos(ServerLinePosVecType pos) { m_LogicLinePos = pos; }
		void SetServerLinePos(PicRateConfigVecType cfg) { m_LogicPicRate = cfg; }
	protected:

    // ��ȡ���õı���
	protected:
		uint32                         m_WeightPicCfg[MAXCOLUMN][MAXWEIGHT];  // Ȩ�����ñ�
		ServerWheelInfoType            m_WheelInfo;                           //ˮ��������ͼ�⼯��
		ServerWheelInfoType            m_FindWheelInfo;                       //ˮ������ͼ�⼯��
		ServerLinePosVecType           m_LogicLinePos;                       //�߶�Ӧ�ĵ�
		PicRateConfigVecType           m_LogicPicRate;                       //ͼƬ��Ӧ������
		uint64                         m_UserBet;                            //���ߵ�Ѻע
		uint32                         m_BonusFreeTimes[MAXCOLUMN];           //bonus��Ѵ���
		int64                         m_FeeRate;                             // ��ˮ����
		int64                         m_JackpotRate;                         // �ʽ����
		int32                         m_ReturnUserRate[2];                   //���˷�������
		int32                         m_ReturnSysRate[5];                    // ϵͳ��������
		bool                           m_IsNewWheel;                       //�Ƿ��и���
		ServerWheelInfoType            m_NoRateWheel;                           //û�����ʵ�ͼ�⼯
	};
}
#endif