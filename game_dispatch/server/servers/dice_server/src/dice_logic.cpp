
#include "dice_logic.h"
#include "svrlib.h"
#include "math/rand_table.h"

using namespace svrlib;

namespace game_dice
{
//�˿�����
	const BYTE CDiceLogic::m_cbDiceData[DICE_POINT_COUNT]={0x01,0x02,0x03,0x04,0x05,0x06};

//////////////////////////////////////////////////////////////////////////

	//���캯��
	CDiceLogic::CDiceLogic()
	{
		m_nSumPoint = 0;
		memset(m_nDice,0,sizeof(m_nDice));
		memset(m_nCountNum,0,sizeof(m_nCountNum));
		m_enNumber_1=m_enNumber_2=m_enNumber_3=m_enNumber_4=m_enNumber_5=m_enNumber_6 = CT_ERROR;

		for (int i=0;i<AREA_COUNT;i++)
		{
			m_nCompensateRatio[i] = 1;
		}

		m_nCompensateRatio[CT_ERROR]	= Multiple_CT_ERROR;

		m_nCompensateRatio[CT_SUM_SMALL] = Multiple_CT_SUM_SMALL;
		m_nCompensateRatio[CT_SUM_BIG] = Multiple_CT_SUM_BIG;

		m_nCompensateRatio[CT_POINT_THREE]		= Multiple_CT_POINT_THREE;
		m_nCompensateRatio[CT_POINT_FOUR]		= Multiple_CT_POINT_FOUR;
		m_nCompensateRatio[CT_POINT_FIVE]		= Multiple_CT_POINT_FIVE;
		m_nCompensateRatio[CT_POINT_SIX]		= Multiple_CT_POINT_SIX;
		m_nCompensateRatio[CT_POINT_SEVEN]		= Multiple_CT_POINT_SEVEN;
		m_nCompensateRatio[CT_POINT_EIGHT]		= Multiple_CT_POINT_EIGHT;
		m_nCompensateRatio[CT_POINT_NINE]		= Multiple_CT_POINT_NINE;
		m_nCompensateRatio[CT_POINT_TEN]		= Multiple_CT_POINT_TEN;
		m_nCompensateRatio[CT_POINT_ELEVEN]		= Multiple_CT_POINT_ELEVEN;
		m_nCompensateRatio[CT_POINT_TWELVE]		= Multiple_CT_POINT_TWELVE;
		m_nCompensateRatio[CT_POINT_THIR_TEEN]	= Multiple_CT_POINT_THIR_TEEN;
		m_nCompensateRatio[CT_POINT_FOUR_TEEN]	= Multiple_CT_POINT_FOUR_TEEN;
		m_nCompensateRatio[CT_POINT_FIF_TEEN]	= Multiple_CT_POINT_FIF_TEEN;
		m_nCompensateRatio[CT_POINT_SIX_TEEN]	= Multiple_CT_POINT_SIX_TEEN;
		m_nCompensateRatio[CT_POINT_SEVEN_TEEN] = Multiple_CT_POINT_SEVEN_TEEN;
		m_nCompensateRatio[CT_POINT_EIGHT_TEEN] = Multiple_CT_POINT_EIGHT_TEEN;

		m_nCompensateRatio[CT_ANY_CICLE_DICE]	= Multiple_CT_ANY_CICLE_DICE;
		m_nCompensateRatio[CT_LIMIT_CICLE_DICE] = Multiple_CT_LIMIT_CICLE_DICE;

		m_nCompensateRatio[CT_ONE]				= Multiple_CT_ONE;
		m_nCompensateRatio[CT_TWO]				= Multiple_CT_TWO;
		m_nCompensateRatio[CT_THREE]			= Multiple_CT_THREE;
		m_nCompensateRatio[CT_FOUR]				= Multiple_CT_FOUR;
		m_nCompensateRatio[CT_FIVE]				= Multiple_CT_FIVE;
		m_nCompensateRatio[CT_SIX]				= Multiple_CT_SIX;

		m_nCompensateRatio[CT_TWICE_ONE]		= Multiple_CT_TWICE_ONE;
		m_nCompensateRatio[CT_TWICE_TWO]		= Multiple_CT_TWICE_TWO;
		m_nCompensateRatio[CT_TWICE_THREE]		= Multiple_CT_TWICE_THREE;
		m_nCompensateRatio[CT_TWICE_FOUR]		= Multiple_CT_TWICE_FOUR;
		m_nCompensateRatio[CT_TWICE_FIVE]		= Multiple_CT_TWICE_FIVE;
		m_nCompensateRatio[CT_TWICE_SIX]		= Multiple_CT_TWICE_SIX;

		m_nCompensateRatio[CT_TRIPLE_ONE]		= Multiple_CT_TRIPLE_ONE;
		m_nCompensateRatio[CT_TRIPLE_TWO]		= Multiple_CT_TRIPLE_TWO;
		m_nCompensateRatio[CT_TRIPLE_THREE]		= Multiple_CT_TRIPLE_THREE;
		m_nCompensateRatio[CT_TRIPLE_FOUR]		= Multiple_CT_TRIPLE_FOUR;
		m_nCompensateRatio[CT_TRIPLE_FIVE]		= Multiple_CT_TRIPLE_FIVE;
		m_nCompensateRatio[CT_TRIPLE_SIX]		= Multiple_CT_TRIPLE_SIX;
	}

	CDiceLogic::~CDiceLogic()
	{

	}

	void CDiceLogic::ResetGameData()
	{
		m_bIsControlDice = false;
		memset(m_cbControlDice, 0, sizeof(m_cbControlDice));

		//m_bIsControlDice = true;
		//for (int i = 0; i<DICE_COUNT; i++)
		//{
		//	m_cbControlDice[i] = 5;
		//}
		//m_cbControlDice[0] = 1;
		//m_cbControlDice[1] = 2;
		//m_cbControlDice[2] = 1;
	}

	//��������
	void CDiceLogic::RandDice(BYTE cbDiceBuffer[], BYTE cbPointNumber)
	{
		//����׼��
		BYTE cbTempDice[DICE_POINT_COUNT];
		memcpy(cbTempDice,m_cbDiceData,sizeof(m_cbDiceData));

		//�����˿�
		BYTE cbRandCount=0,cbPosition=0;
		do
		{
			cbPosition=g_RandGen.RandUInt()%(cbPointNumber-cbRandCount);

			cbDiceBuffer[cbRandCount++]=cbTempDice[cbPosition];     //�������ȡ���ĵ���
			cbTempDice[cbPosition]=cbTempDice[cbPointNumber-cbRandCount]; //���һ��ǰ�Ƶ������ߵ�λ��
		} while (cbRandCount<cbPointNumber);
	}

	//ҡ��������(��������cbDiceBuffer�ܱ�֤���ͺͽ��������һ��)
	void CDiceLogic::ShakeRandDice(BYTE cbDiceBuffer[], BYTE cbDiceCount)
	{
		memset(m_nCountNum,0,sizeof(m_nCountNum));

		int nRandIndex=0;
		BYTE cbDice[DICE_POINT_COUNT];
		RandDice(cbDice,DICE_POINT_COUNT);

		for(int i=0; i<cbDiceCount; i++) //��������
		{
			nRandIndex = g_RandGen.RandUInt()%DICE_POINT_COUNT;
			m_nDice[i] = cbDice[nRandIndex];
			cbDiceBuffer[i] = cbDice[nRandIndex];
		}
		if (m_bIsControlDice)
		{
			for (int i = 0; i<cbDiceCount; i++)
			{
				m_nDice[i] = m_cbControlDice[i];
				cbDiceBuffer[i] = m_cbControlDice[i];
			}
		}
		/*
		bool bWaidice=false;
		if(cbDice[0]==cbDice[1] && cbDice[1]==cbDice[2])
		{
			bWaidice=true;
		}

		if (bWaidice==false && IsCoupleDice(m_nDice)==true) //���ٷ�Χ����˫��
		{
			int nLoops = 0;
			int nRandNum = g_RandGen.RandUInt()%100;
			if (nRandNum<80)
			{
				do
				{
					for(int i=0; i<cbDiceCount; i++) //��������
					{
						nRandIndex = g_RandGen.RandUInt()%DICE_POINT_COUNT;
						m_nDice[i] = cbDice[nRandIndex];
						cbDiceBuffer[i] = cbDice[nRandIndex];
					}
				} while (nLoops<CONTROL_TRY_TIMES && IsCoupleDice(m_nDice)==true);
			}
		}
		*/
	}

	bool CDiceLogic::IsCoupleDice(BYTE cbDiceData[])
	{
		if ((cbDiceData[0]==cbDiceData[1] || cbDiceData[0]==cbDiceData[2] || cbDiceData[1]==cbDiceData[2]))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	//�������ӽ��
	void CDiceLogic::ComputeDiceResult()
	{
		m_enNumber_1=CT_ERROR;
		m_enNumber_2=CT_ERROR;
		m_enNumber_3=CT_ERROR;
		m_enNumber_4=CT_ERROR;
		m_enNumber_5=CT_ERROR;
		m_enNumber_6=CT_ERROR;

		m_nSumPoint = GetDicePoint(m_nDice);

		CountSameDice(m_nDice);
	}

	//��ȡ������
	enDiceType CDiceLogic::GetDicePoint(BYTE nDiceArray[])
	{
		int nDicePoints = 0;
		for (int i=0;i<DICE_COUNT;i++)
		{
			nDicePoints += nDiceArray[i]; //���������ӵ�����
		}

		if (nDicePoints>10)
		{
			m_enBigSmall = CT_SUM_BIG;
		}
		else
		{
			m_enBigSmall = CT_SUM_SMALL;
		}

		switch(nDicePoints)
		{
			case 3:  return CT_POINT_THREE;
			case 4:  return CT_POINT_FOUR;
			case 5:  return CT_POINT_FIVE;
			case 6:  return CT_POINT_SIX;
			case 7:  return CT_POINT_SEVEN;
			case 8:  return CT_POINT_EIGHT;
			case 9:  return CT_POINT_NINE;
			case 10: return CT_POINT_TEN;
			case 11: return CT_POINT_ELEVEN;
			case 12: return CT_POINT_TWELVE;
			case 13: return CT_POINT_THIR_TEEN;
			case 14: return CT_POINT_FOUR_TEEN;
			case 15: return CT_POINT_FIF_TEEN;
			case 16: return CT_POINT_SIX_TEEN;
			case 17: return CT_POINT_SEVEN_TEEN;
			case 18: return CT_POINT_EIGHT_TEEN;
			default: return CT_POINT_THREE;
		}
	}

	//��ȡ��С
	enDiceType CDiceLogic::GetBigSmall(BYTE nDiceArray[])
	{
		int nDicePoints = 0;
		for (int i=0;i<DICE_COUNT;i++)
		{
			nDicePoints += nDiceArray[i]; //���������ӵ�����
		}

		if (nDicePoints>=3 && nDicePoints<=10) //3--10:С��11--18:��
		{
			return CT_SUM_SMALL;  //С
		}
		else
		{
			return CT_SUM_BIG;    //��
		}
	}

	//ͳ����ͬ��������
	enDiceType CDiceLogic::GetThreeSameDice(BYTE nDiceArray[])
	{
		if (1==nDiceArray[1] && 1==nDiceArray[1] && 1==nDiceArray[2])
		{
			return CT_TRIPLE_ONE;   //����1
		}
		else if(2==nDiceArray[1] && 2==m_nDice[1] && 2==nDiceArray[2])
		{
			return CT_TRIPLE_TWO;   //����2
		}
		else if(3==nDiceArray[1] && 3==nDiceArray[1] && 3==nDiceArray[2])
		{
			return CT_TRIPLE_THREE; //����3
		}
		else if(4==nDiceArray[1] && 4==nDiceArray[1] && 4==nDiceArray[2])
		{
			return CT_TRIPLE_FOUR;  //����4
		}
		else if(5==nDiceArray[1] && 5==nDiceArray[1] && 5==nDiceArray[2])
		{
			return CT_TRIPLE_FIVE;  //����5
		}
		else if(6==nDiceArray[1] && 6==nDiceArray[1] && 6==nDiceArray[2])
		{
			return CT_TRIPLE_SIX;   //����6
		}
		return CT_ERROR;
	}


	//ͳ����ͬ����
	void CDiceLogic::CountSameDice(BYTE nDiceArray[])
	{
		memset(m_nCountNum,0,sizeof(m_nCountNum));

		for (int i=0;i<DICE_COUNT;i++) //ͳ�Ƹ������
		{
			if (1==nDiceArray[i])
			{
				m_nCountNum[0] += 1;
			}
			else if (2==nDiceArray[i])
			{
				m_nCountNum[1] += 1;
			}
			else if (3==nDiceArray[i])
			{
				m_nCountNum[2] += 1;
			}
			else if (4==nDiceArray[i])
			{
				m_nCountNum[3] += 1;
			}
			else if (5==nDiceArray[i])
			{
				m_nCountNum[4] += 1;
			}
			else if (6==nDiceArray[i])
			{
				m_nCountNum[5] += 1;
			}
		}

		switch(m_nCountNum[0]) //1�����
		{
			case 0: m_enNumber_1 = CT_ERROR;     break;
			case 1: m_enNumber_1 = CT_ONE;       break;
			case 2: m_enNumber_1 = CT_TWICE_ONE; break;
			case 3: m_enNumber_1 = CT_TRIPLE_ONE; return;//1��Χ��
			default: m_enNumber_1 = CT_ERROR;
		}

		switch(m_nCountNum[1])//2�����
		{
			case 0: m_enNumber_2 = CT_ERROR;     break;
			case 1: m_enNumber_2 = CT_TWO;       break;
			case 2: m_enNumber_2 = CT_TWICE_TWO; break;
			case 3: m_enNumber_2 = CT_TRIPLE_TWO; return;//2��Χ��
			default: m_enNumber_2 = CT_ERROR;
		}

		switch(m_nCountNum[2])//3�����
		{
			case 0: m_enNumber_3 = CT_ERROR;       break;
			case 1: m_enNumber_3 = CT_THREE;       break;
			case 2: m_enNumber_3 = CT_TWICE_THREE; break;
			case 3: m_enNumber_3 = CT_TRIPLE_THREE; return;//3��Χ��
			default: m_enNumber_3 = CT_ERROR;
		}

		switch(m_nCountNum[3])//4�����
		{
			case 0: m_enNumber_4 = CT_ERROR;       break;
			case 1: m_enNumber_4 = CT_FOUR;        break;
			case 2: m_enNumber_4 = CT_TWICE_FOUR;  break;
			case 3: m_enNumber_4 = CT_TRIPLE_FOUR; return;//4��Χ��
			default: m_enNumber_4 = CT_ERROR;
		}

		switch(m_nCountNum[4])//5�����
		{
			case 0: m_enNumber_5 = CT_ERROR;       break;
			case 1: m_enNumber_5 = CT_FIVE;        break;
			case 2: m_enNumber_5 = CT_TWICE_FIVE;  break;
			case 3: m_enNumber_5 = CT_TRIPLE_FIVE; return;//5��Χ��
			default: m_enNumber_5= CT_ERROR;
		}

		switch(m_nCountNum[5])//6�����
		{
			case 0: m_enNumber_6 = CT_ERROR;       break;
			case 1: m_enNumber_6 = CT_SIX;         break;
			case 2: m_enNumber_6 = CT_TWICE_SIX;   break;
			case 3: m_enNumber_6 = CT_TRIPLE_SIX;  return;//6��Χ��
			default: m_enNumber_6 = CT_ERROR;
		}
	}

	//Χ��
	bool CDiceLogic::IsWaidice()
	{
		if (CT_TRIPLE_ONE  == m_enNumber_1 ||
			CT_TRIPLE_TWO  == m_enNumber_2 ||
			CT_TRIPLE_THREE== m_enNumber_3 ||
			CT_TRIPLE_FOUR == m_enNumber_4 ||
			CT_TRIPLE_FIVE == m_enNumber_5 ||
			CT_TRIPLE_SIX  == m_enNumber_6)
		{
			return true; //Χ��ͨ��
		}

		return false;
	}

	bool CDiceLogic::CompareHitPrize(int nBetIndex,int & nMultiple)
	{
		nMultiple = Multiple_CT_ERROR;
		switch(nBetIndex)
		{
		case CT_SUM_SMALL:  //ѹС
			{
				nMultiple = Multiple_CT_SUM_SMALL;

				if (IsWaidice()==true)
				{
					return false;
				}

				if (m_enBigSmall == CT_SUM_SMALL)
				{
					return true;
				}
				return false;
			}
		case CT_SUM_BIG:   //ѹ��
			{
				nMultiple = Multiple_CT_SUM_BIG;

				if (IsWaidice()==true)
				{
					return false;
				}

				if (m_enBigSmall == CT_SUM_BIG)
				{
					return true;
				}
				return false;
			}
			/////////////////////////////////////////////////////////////////////////////////////
		case CT_ONE:
			{
				if (m_enNumber_1 == CT_ONE)
				{
					//nMultiple = Multiple_CT_ONE;
					nMultiple = 1;
					return true;
				}
				else if (m_enNumber_1 == CT_TWICE_ONE)
				{
					nMultiple = 2;
					return true;
				}
				else if (m_enNumber_1 == CT_TRIPLE_ONE)
				{
					nMultiple = 3;
					return true;
				}
				nMultiple = 1;
				return false;
			}
		case CT_TWO:
			{
				if (m_enNumber_2 == CT_TWO)
				{
					nMultiple = 1;
					return true;
				}
				else if (m_enNumber_2 == CT_TWICE_TWO)
				{
					nMultiple = 2;
					return true;
				}
				else if (m_enNumber_2 == CT_TRIPLE_TWO)
				{
					nMultiple = 3;
					return true;
				}
				nMultiple = 1;
				return false;
			}
		case CT_THREE:
			{
				if (m_enNumber_3 == CT_THREE)
				{
					nMultiple = 1;
					return true;
				}
				else if (m_enNumber_3 == CT_TWICE_THREE)
				{
					nMultiple = 2;
					return true;
				}
				else if (m_enNumber_3 == CT_TRIPLE_THREE)
				{
					nMultiple = 3;
					return true;
				}
				nMultiple = 1;
				return false;
			}
		case CT_FOUR:
			{
				if (m_enNumber_4 == CT_FOUR)
				{
					nMultiple = 1;
					return true;
				}
				else if (m_enNumber_4 == CT_TWICE_FOUR)
				{
					nMultiple = 2;
					return true;
				}
				else if (m_enNumber_4 == CT_TRIPLE_FOUR)
				{
					nMultiple = 3;
					return true;
				}
				nMultiple = 1;
				return false;
			}
		case CT_FIVE:
			{
				if (m_enNumber_5 == CT_FIVE)
				{
					nMultiple = 1;
					return true;
				}
				else if (m_enNumber_5 == CT_TWICE_FIVE)
				{
					nMultiple = 2;
					return true;
				}
				else if (m_enNumber_5 == CT_TRIPLE_FIVE)
				{
					nMultiple = 3;
					return true;
				}
				nMultiple = 1;
				return false;
			}
		case CT_SIX:
			{
				if (m_enNumber_6 == CT_SIX)
				{
					nMultiple = 1;
					return true;
				}
				else if (m_enNumber_6 == CT_TWICE_SIX)
				{
					nMultiple = 2;
					return true;
				}
				else if (m_enNumber_6 == CT_TRIPLE_SIX)
				{
					nMultiple = 3;
					return true;
				}
				nMultiple = 1;
				return false;
			}
			/////////////////////////////////////////////////////////////////////////////////////
		case CT_TWICE_ONE:
			{
				nMultiple = Multiple_CT_TWICE_ONE;
				//if (IsWaidice() == true)
				//{
				//	return false;
				//}
				if (m_enNumber_1 == CT_TWICE_ONE)
				{
					return true;
				}
				else if (m_enNumber_1 == CT_TRIPLE_ONE)
				{
					return true;
				}
				return false;
			}
		case CT_TWICE_TWO:
			{
				nMultiple = Multiple_CT_TWICE_TWO;
				//if (IsWaidice() == true)
				//{
				//	return false;
				//}
				if (m_enNumber_2 == CT_TWICE_TWO)
				{
					return true;
				}
				else if (m_enNumber_2 == CT_TRIPLE_TWO)
				{
					return true;
				}
				return false;
			}
		case CT_TWICE_THREE:
			{
				nMultiple = Multiple_CT_TWICE_THREE;
				//if (IsWaidice() == true)
				//{
				//	return false;
				//}
				if (m_enNumber_3 == CT_TWICE_THREE)
				{
					return true;
				}
				else if (m_enNumber_3 == CT_TRIPLE_THREE)
				{
					return true;
				}
				return false;
			}
		case CT_TWICE_FOUR:
			{
				nMultiple = Multiple_CT_TWICE_FOUR;
				//if (IsWaidice() == true)
				//{
				//	return false;
				//}
				if (m_enNumber_4 == CT_TWICE_FOUR)
				{
					return true;
				}
				else if (m_enNumber_4 == CT_TRIPLE_FOUR)
				{
					return true;
				}
				return false;
			}
		case CT_TWICE_FIVE:
			{
				nMultiple = Multiple_CT_TWICE_FIVE;
				//if (IsWaidice() == true)
				//{
				//	return false;
				//}
				if (m_enNumber_5 == CT_TWICE_FIVE)
				{
					return true;
				}
				else if (m_enNumber_5 == CT_TRIPLE_FIVE)
				{
					return true;
				}
				return false;
			}
		case CT_TWICE_SIX:
			{
				nMultiple = Multiple_CT_TWICE_SIX;
				//if (IsWaidice()==true)
				//{
				//	return false;
				//}
				if (m_enNumber_6 == CT_TWICE_SIX)
				{
					return true;
				}
				else if (m_enNumber_6 == CT_TRIPLE_SIX)
				{
					return true;
				}
				return false;
			}
			/////////////////////������////////////////////////////
		case CT_POINT_FOUR:  //4��
			{
				nMultiple = Multiple_CT_POINT_FOUR;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_FOUR)
				{
					return true;
				}
				return false;
			}
		case CT_POINT_FIVE:  //5��
			{
				nMultiple = Multiple_CT_POINT_FIVE;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_FIVE)
				{
					return true;
				}
				return false;
			}
		case CT_POINT_SIX:  //6��
			{
				nMultiple = Multiple_CT_POINT_SIX;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_SIX)
				{
					return true;
				}
				return false;
			}
		case CT_POINT_SEVEN:  //7��
			{
				nMultiple = Multiple_CT_POINT_SEVEN;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_SEVEN)
				{
					return true;
				}
				return false;
			}
		case CT_POINT_EIGHT:  //8��
			{
				nMultiple = Multiple_CT_POINT_EIGHT;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_EIGHT)
				{
					return true;
				}
				return false;
			}
		case CT_POINT_NINE:  //9��
			{
				nMultiple = Multiple_CT_POINT_NINE;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_NINE)
				{
					return true;
				}
				return false;
			}
		case CT_POINT_TEN:  //10��
			{
				nMultiple = Multiple_CT_POINT_TEN;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_TEN)
				{
					return true;
				}
				return false;
			}
		case CT_POINT_ELEVEN:  //11��
			{
				nMultiple = Multiple_CT_POINT_ELEVEN;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_ELEVEN)
				{
					return true;
				}
				return false;
			}
		case CT_POINT_TWELVE: //12��
			{
				nMultiple = Multiple_CT_POINT_TWELVE;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_TWELVE)
				{
					return true;
				}
				return false;
			}
		case CT_POINT_THIR_TEEN:  //13��
			{
				nMultiple = Multiple_CT_POINT_THIR_TEEN;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_THIR_TEEN)
				{
					return true;
				}
				return false;
			}
		case CT_POINT_FOUR_TEEN:  //14��
			{
				nMultiple = Multiple_CT_POINT_FOUR_TEEN;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_FOUR_TEEN)
				{
					return true;
				}
				return false;
			}
		case CT_POINT_FIF_TEEN:  //15��
			{
				nMultiple = Multiple_CT_POINT_FIF_TEEN;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_FIF_TEEN)
				{
					return true;
				}
				return false;
			}
		case CT_POINT_SIX_TEEN:  //16��
			{
				nMultiple = Multiple_CT_POINT_SIX_TEEN;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_SIX_TEEN)
				{
					return true;
				}
				return false;
			}
		case CT_POINT_SEVEN_TEEN:  //17��
			{
				nMultiple = Multiple_CT_POINT_SEVEN_TEEN;
				if (IsWaidice() == true)
				{
					return false;
				}
				if (m_nSumPoint == CT_POINT_SEVEN_TEEN)
				{
					return true;
				}
				return false;
			}
			/////////////////////////////////////////////////////////////////////////////////////

		case CT_ANY_CICLE_DICE: //����Χ��
			{
				nMultiple = Multiple_CT_ANY_CICLE_DICE;

				if (IsWaidice()==true)
				{
					return true;
				}
				return false;
			}
		case CT_TRIPLE_ONE:  //1��ָ��Χ��
			{
				nMultiple = Multiple_CT_LIMIT_CICLE_DICE;

				if (m_enNumber_1 == CT_TRIPLE_ONE)
				{
					return true;
				}
				return false;
			}
		case CT_TRIPLE_TWO:  //2��ָ��Χ��
			{
				nMultiple = Multiple_CT_LIMIT_CICLE_DICE;

				if (m_enNumber_2 == CT_TRIPLE_TWO)
				{
					return true;
				}
				return false;
			}
		case CT_TRIPLE_THREE:  //3��ָ��Χ��
			{
				nMultiple = Multiple_CT_LIMIT_CICLE_DICE;

				if (m_enNumber_3 == CT_TRIPLE_THREE)
				{
					return true;
				}
				return false;
			}
		case CT_TRIPLE_FOUR:  //4��ָ��Χ��
			{
				nMultiple = Multiple_CT_LIMIT_CICLE_DICE;

				if (m_enNumber_4 == CT_TRIPLE_FOUR)
				{
					return true;
				}
				return false;
			}
		case CT_TRIPLE_FIVE:  //5��ָ��Χ��
			{
				nMultiple = Multiple_CT_LIMIT_CICLE_DICE;

				if (m_enNumber_5 == CT_TRIPLE_FIVE)
				{
					return true;
				}
				return false;
			}
		case CT_TRIPLE_SIX:  //6��ָ��Χ��
			{
				nMultiple = Multiple_CT_LIMIT_CICLE_DICE;

				if (m_enNumber_6 == CT_TRIPLE_SIX)
				{
					return true;
				}
				return false;
			}
			default:
			{
				nMultiple = Multiple_CT_ERROR;
				return false;
			}
		}

	return false;
}

};


