
#include "niuniu_logic.h"
#include "svrlib.h"
#include "math/rand_table.h"

using namespace svrlib;

namespace game_niuniu
{
//�˿�����
const BYTE CNiuNiuLogic::m_cbCardListData[CARD_COUNT]=
{
	    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//���� A - K
		0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//÷�� A - K
		0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//���� A - K
		0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//���� A - K		
};

//////////////////////////////////////////////////////////////////////////

//���캯��
CNiuNiuLogic::CNiuNiuLogic()
{
}

//��������
CNiuNiuLogic::~CNiuNiuLogic()
{
}

//�����˿�
void CNiuNiuLogic::RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount)
{
	//����׼��
	BYTE cbCardData[CARD_COUNT];
	memcpy(cbCardData,m_cbCardListData,sizeof(m_cbCardListData));

	//�����˿�
	BYTE cbRandCount=0,cbPosition=0;
	do
	{
		cbPosition=g_RandGen.RandUInt()%(CARD_COUNT-cbRandCount);
		cbCardBuffer[cbRandCount++]=cbCardData[cbPosition];
		cbCardData[cbPosition]=cbCardData[CARD_COUNT-cbRandCount];
	} while (cbRandCount<cbBufferCount);

	return;
}

void CNiuNiuLogic::RandCardListEx(BYTE cbCardBuffer[], BYTE cbBufferCount)
{
	BYTE *cbCardData=new BYTE[cbBufferCount];
	memcpy(cbCardData,cbCardBuffer,sizeof(BYTE)*cbBufferCount);

	//�����˿�
	BYTE cbRandCount=0,cbPosition=0;
	do
	{
		cbPosition=g_RandGen.RandUInt()%(cbBufferCount-cbRandCount);
		cbCardBuffer[cbRandCount++]=cbCardData[cbPosition];
		cbCardData[cbPosition]=cbCardData[cbBufferCount-cbRandCount];
	} while (cbRandCount<cbBufferCount);
	delete[] cbCardData;
}

int CNiuNiuLogic::RetType(int itype)
{
	itype = itype%10;
	switch(itype)
	{
	case 0:
		return CT_SPECIAL_NIUNIU;
	case 1:
		return CT_SPECIAL_NIU1;
		break;
	case 2:
		return CT_SPECIAL_NIU2;
		break;
	case 3:
		return CT_SPECIAL_NIU3;
		break;
	case 4:
		return CT_SPECIAL_NIU4;
		break;
	case 5:
		return CT_SPECIAL_NIU5;
		break;
	case 6:
		return CT_SPECIAL_NIU6;
		break;
	case 7:
		return CT_SPECIAL_NIU7;
		break;
	case 8:
		return CT_SPECIAL_NIU8;
		break;
	case 9:
		return CT_SPECIAL_NIU9;
		break;
	default :
		return CT_POINT;
		break;
	}
}
//��ȡ����
BYTE CNiuNiuLogic::GetCardType(const BYTE cbCardData[], BYTE cbCardCount,BYTE *bcOutCadData )
{
	//�Ϸ��ж�
	ASSERT(5==cbCardCount);
	if (5!=cbCardCount) return CT_ERROR;

	//�����˿�
	BYTE cbCardDataSort[CARD_COUNT];
	memcpy(cbCardDataSort,cbCardData,sizeof(BYTE)*cbCardCount);
	SortCardList(cbCardDataSort,cbCardCount,ST_NEW);

	if(bcOutCadData != NULL)
	{
		memcpy(bcOutCadData,cbCardDataSort,cbCardCount);
	}
	int igetW= 0;
	if(GetCardNewValue(cbCardDataSort[0])==GetCardNewValue(cbCardDataSort[cbCardCount-2]))
	{
		if(bcOutCadData != NULL)
		{
			memcpy(bcOutCadData,cbCardDataSort,cbCardCount);
		}
		return CT_SPECIAL_BOMEBOME;
	}else
	{
		if(GetCardNewValue(cbCardDataSort[1])==GetCardNewValue(cbCardDataSort[cbCardCount-1]))
		{
			if(bcOutCadData != NULL)
			{
				memcpy(bcOutCadData,cbCardDataSort,cbCardCount);
			}
			return CT_SPECIAL_BOMEBOME;
		}

	}
    //�ж���Сţ
    bool    bFlag = true;
    uint32  tenCount = 0;
    for(uint8 i=0;i<cbCardCount;++i)
    {
        uint32 v = GetCardLogicValue(cbCardData[i]);
        tenCount += v;
        if(v > 4 || tenCount > 10){
            bFlag = false;
            break;
        }        
    }
    if(bFlag){
        return CT_SPECIAL_NIUNIUXW;
    }
    //�ж��廨ţ
    bFlag = true;
    for(uint8 i=0;i<cbCardCount;++i)
    {
        if(GetCardLogicValue(cbCardData[i]) != 10 || GetCardValue(cbCardData[i]) == 10)
        {
            bFlag = false;
            break;
        }        
    }
    if(bFlag){
        return CT_SPECIAL_NIUNIUDW;
    }     
    
	if(GetCardColor(cbCardDataSort[0])==0x04&&GetCardColor(cbCardDataSort[1])==0x04)
	{
		if(GetCardNewValue(cbCardDataSort[2])==GetCardNewValue(cbCardDataSort[3]))
		{
			if(bcOutCadData != NULL)
			{
				bcOutCadData[0] = cbCardDataSort[2];
				bcOutCadData[1] = cbCardDataSort[3];
				bcOutCadData[2] = cbCardDataSort[0];
				bcOutCadData[3] = cbCardDataSort[1];
				bcOutCadData[4] = cbCardDataSort[4];

			}
			return CT_SPECIAL_BOMEBOME;
		}else
		{
			if(GetCardNewValue(cbCardDataSort[3])==GetCardNewValue(cbCardDataSort[4]))
			{
				if(bcOutCadData != NULL)
				{
					bcOutCadData[0] = cbCardDataSort[0];
					bcOutCadData[1] = cbCardDataSort[1];
					bcOutCadData[2] = cbCardDataSort[3];
					bcOutCadData[3] = cbCardDataSort[4];
					bcOutCadData[4] = cbCardDataSort[2];

				}
				return CT_SPECIAL_BOMEBOME;
			}

		}	

	}
	if(GetCardColor(cbCardDataSort[0])==0x04)
	{
		if(GetCardNewValue(cbCardDataSort[1])==GetCardNewValue(cbCardDataSort[3]))
		{
			if(bcOutCadData != NULL)
			{
				bcOutCadData[0] = cbCardDataSort[1];
				bcOutCadData[1] = cbCardDataSort[2];
				bcOutCadData[2] = cbCardDataSort[3];
				bcOutCadData[3] = cbCardDataSort[0];
				bcOutCadData[4] = cbCardDataSort[4];

			}
			return CT_SPECIAL_BOMEBOME;
		}else
		{
			if(GetCardNewValue(cbCardDataSort[2])==GetCardNewValue(cbCardDataSort[4]))
			{
				if(bcOutCadData != NULL)
				{
					bcOutCadData[0] = cbCardDataSort[2];
					bcOutCadData[1] = cbCardDataSort[3];
					bcOutCadData[2] = cbCardDataSort[4];
					bcOutCadData[3] = cbCardDataSort[0];
					bcOutCadData[4] = cbCardDataSort[1];

				}
				return CT_SPECIAL_BOMEBOME;
			}

		}	

	}
	bool blBig = true;
	int iCount = 0;
	int iLogicValue = 0;
	int iValueTen = 0;
	for (int i = 0;i<cbCardCount;i++)
	{
		BYTE bcGetValue = GetCardLogicValue(cbCardDataSort[i]);
		if(bcGetValue!=10&&bcGetValue!=11)
		{

			blBig = false;
			break;

		}else
		{
			if(GetCardNewValue(cbCardDataSort[i])==10)
			{
				iValueTen++;
			}
		}
		iCount++;
	}
	/*if(blBig)
	{
		if(bcOutCadData != NULL)
		{
			CopyMemory(bcOutCadData,cbCardDataSort,cbCardCount);
		}
		if(iValueTen==0)
			return CT_SPECIAL_NIUKING;
		else
		{
			if(iValueTen==1)
			{
				return CT_SPECIAL_NIUYING;
			}
		}
	}*/

	int n = 0;

	BYTE bcMakeMax[5];
	memset(bcMakeMax,0,5);
	int iBigValue = 0;
	BYTE iSingleA[2];
	int iIndex = 0;
	bcMakeMax[0]= cbCardDataSort[n];

	int iGetTenCount = 0;

	for (int iten = 0;iten<cbCardCount;iten++)
	{
		if(GetCardLogicValue(cbCardDataSort[iten])==10||GetCardLogicValue(cbCardDataSort[iten])==11)
		{
			iGetTenCount++;

		}
	}
	if( iGetTenCount>=3)
	{
		if(GetCardColor(cbCardDataSort[0])==0x04&&GetCardColor(cbCardDataSort[1])==0x04)
		{
			if(bcOutCadData != NULL)
			{
				bcOutCadData[0] = cbCardDataSort[0];
				bcOutCadData[1] = cbCardDataSort[3];
				bcOutCadData[2] = cbCardDataSort[4];
				bcOutCadData[3] = cbCardDataSort[1];
				bcOutCadData[4] = cbCardDataSort[2];

			}
			return CT_SPECIAL_NIUNIUDW;

		}
		if(GetCardColor(cbCardDataSort[0])==0x04)
		{
			//��С������С����ϳ�ţ 
			if(bcOutCadData != NULL)
			{
				bcOutCadData[0] = cbCardDataSort[0];
				bcOutCadData[1] = cbCardDataSort[3];
				bcOutCadData[2] = cbCardDataSort[4];
				bcOutCadData[3] = cbCardDataSort[1];
				bcOutCadData[4] = cbCardDataSort[2];
			}
			if(cbCardDataSort[0]==0x42)
				return CT_SPECIAL_NIUNIUDW;
			else
				return CT_SPECIAL_NIUNIUXW;
		}else
		{
			return RetType(GetCardLogicValue(cbCardDataSort[3])+GetCardLogicValue(cbCardDataSort[4]));
		}

	}
	if(iGetTenCount==2||(iGetTenCount==1&&GetCardColor(cbCardDataSort[0])==0x04))
	{

		if(GetCardColor(cbCardDataSort[0])==0x04&&GetCardColor(cbCardDataSort[1])==0x04)
		{
			if(bcOutCadData != NULL)
			{
				bcOutCadData[0] = cbCardDataSort[0];
				bcOutCadData[1] = cbCardDataSort[3];
				bcOutCadData[2] = cbCardDataSort[4];
				bcOutCadData[3] = cbCardDataSort[1];
				bcOutCadData[4] = cbCardDataSort[2];
			}
			return CT_SPECIAL_NIUNIUDW;
		}else
		{
			//�����һ���� ���������������Ϊ10����ţţ
			if(GetCardColor(cbCardDataSort[0])==0x04)
			{

				for ( n=1;n<cbCardCount;n++)
				{
					for (int j = 1;j<cbCardCount;j++)
					{
						if(j != n)
						{
							for (int w = 1;w<cbCardCount;w++)
							{
								if(w != n&&w!=j)
								{
									//���ʣ����������������������λ10��������

									if((GetCardLogicValue(cbCardDataSort[n])+GetCardLogicValue(cbCardDataSort[j])+GetCardLogicValue(cbCardDataSort[w]))%10==0)
									{

										int add = 0;
										for (int y = 1;y<cbCardCount;y++)
										{
											if(y != n&&y!=j&&y!=w)
											{
												iSingleA[add] =cbCardDataSort[y]; 
												add++;

											}

										}
										if(bcOutCadData != NULL)
										{
											bcOutCadData[0] = cbCardDataSort[n];
											bcOutCadData[1] = cbCardDataSort[j];
											bcOutCadData[2] = cbCardDataSort[w];
											bcOutCadData[3] = cbCardDataSort[0];
											bcOutCadData[4] = iSingleA[0];
										}
										if(cbCardDataSort[0]==0x42)
											return CT_SPECIAL_NIUNIUDW;
										else
											return CT_SPECIAL_NIUNIUXW;


									}
								}
							}
						}
					}
				}
				//�����һ���� ��������������ϲ�Ϊ10�� ȡ���ŵ����������
				BYTE bcTmp[4];
				int iBig = 0;
				int in = 0;
				for ( in = 1;in<cbCardCount;in++)
				{
					for (int j = 1;j<cbCardCount;j++)
					{
						if(in != j)
						{
							BYTE bclogic = (GetCardLogicValue(cbCardDataSort[in])+GetCardLogicValue(cbCardDataSort[j]))%10;
							if(bclogic>iBig)
							{
								iBig = bclogic;
								int add = 0;
								bcTmp[0]=cbCardDataSort[in];
								bcTmp[1]=cbCardDataSort[j];
								for (int y = 1;y<cbCardCount;y++)
								{
									if(y != in&&y!=j)
									{
										iSingleA[add] =cbCardDataSort[y]; 
										add++;
									}

								}
								bcTmp[2]=iSingleA[0];
								bcTmp[3]=iSingleA[1];

							}

						}
					}   
				}

				if(bcOutCadData != NULL)
				{
					bcOutCadData[0] = cbCardDataSort[0];
					bcOutCadData[1] = bcTmp[2];
					bcOutCadData[2] = bcTmp[3];
					bcOutCadData[3] = bcTmp[0];
					bcOutCadData[4] = bcTmp[1];
				}
				if(iGetTenCount==1&&GetCardColor(cbCardDataSort[0])==0x04)
				{
					//���滹����� ������Ϊ 10 Ҳ������ϳ�ţţ

				}else
				{
					//���û����Ƚ� ������С��������������
					return RetType(GetCardLogicValue(bcTmp[0])+GetCardLogicValue(bcTmp[1]));
				}


			}else
			{
				if((GetCardLogicValue(cbCardDataSort[2])+GetCardLogicValue(cbCardDataSort[3])+GetCardLogicValue(cbCardDataSort[4]))%10==0)
				{
					if(bcOutCadData != NULL)
					{
						bcOutCadData[0] = cbCardDataSort[2];
						bcOutCadData[1] = cbCardDataSort[3];
						bcOutCadData[2] = cbCardDataSort[4];
						bcOutCadData[3] = cbCardDataSort[0];
						bcOutCadData[4] = cbCardDataSort[1];
					}
					return CT_SPECIAL_NIUNIU;                    
				}else
				{
					for ( n= 2;n<cbCardCount;n++)
					{
						for (int j = 2;j<cbCardCount;j++)
						{
							if(j != n)
							{
								if((GetCardLogicValue(cbCardDataSort[n])+GetCardLogicValue(cbCardDataSort[j]))%10==0)
								{
									int add = 0;
									for (int y = 2;y<cbCardCount;y++)
									{
										if(y != n&&y!=j)
										{
											iSingleA[add] =cbCardDataSort[y]; 
											add++;

										}
									}
									if(iBigValue<=iSingleA[0]%10)
									{
										iBigValue = GetCardLogicValue(iSingleA[0])%10;
										if(bcOutCadData != NULL)
										{
											bcOutCadData[0]= cbCardDataSort[0];
											bcOutCadData[1]= cbCardDataSort[n]; 
											bcOutCadData[2]= cbCardDataSort[j]; 
											bcOutCadData[3]= cbCardDataSort[1];
											bcOutCadData[4]= iSingleA[0]; 

										}

										if(iBigValue==0)
										{
											return CT_SPECIAL_NIUNIU;                                            
										}
									}

								}
							}
						}
					}
					if(iBigValue != 0)
					{
						return RetType(iBigValue);
					}
				}
			}

		}

		iGetTenCount = 1;

	}
	//4�����
	if(iGetTenCount==1)
	{
		if(GetCardColor(cbCardDataSort[0])==0x04)
		{
			for ( n= 1;n<cbCardCount;n++)
			{
				for (int j = 1;j<cbCardCount;j++)
				{
					if(j != n)
					{
						//����������ϳ�ţ
						if((GetCardLogicValue(cbCardDataSort[n])+GetCardLogicValue(cbCardDataSort[j]))%10==0)
						{
							int add = 0;
							for (int y = 1;y<cbCardCount;y++)
							{
								if(y != n&&y!=j)
								{
									iSingleA[add] =cbCardDataSort[y]; 
									add++;

								}

							}

							if(bcOutCadData != NULL)
							{
								bcOutCadData[0] = cbCardDataSort[0];
								bcOutCadData[1] = iSingleA[0];
								bcOutCadData[2] = iSingleA[1];
								bcOutCadData[3] = cbCardDataSort[n];
								bcOutCadData[4] = cbCardDataSort[j];
							}
							if(cbCardDataSort[0]==0x42)
								return CT_SPECIAL_NIUNIUDW;
							else
								return CT_SPECIAL_NIUNIUXW;

						}
					}

				}
			}

			//ȡ4����������ĵ���

			BYTE bcTmp[4];
			int iBig = 0;
			int in = 0;
			for ( in = 1;in<cbCardCount;in++)
			{
				for (int j = 1;j<cbCardCount;j++)
				{
					if(in != j)
					{
						BYTE bclogic = (GetCardLogicValue(cbCardDataSort[in])+GetCardLogicValue(cbCardDataSort[j]))%10;
						if(bclogic>iBig)
						{
							iBig = bclogic;
							int add = 0;
							bcTmp[0]=cbCardDataSort[in];
							bcTmp[1]=cbCardDataSort[j];
							for (int y = 1;y<cbCardCount;y++)
							{
								if(y != in&&y!=j)
								{
									iSingleA[add] =cbCardDataSort[y]; 
									add++;
								}

							}
							bcTmp[2]=iSingleA[0];
							bcTmp[3]=iSingleA[1];

						}

					}
				}   
			}

			if(bcOutCadData != NULL)
			{
				bcOutCadData[0] = cbCardDataSort[0];
				bcOutCadData[1] = bcTmp[2];
				bcOutCadData[2] = bcTmp[3];
				bcOutCadData[3] = bcTmp[0];
				bcOutCadData[4] = bcTmp[1];
			}
			return RetType(GetCardLogicValue(bcTmp[0])+GetCardLogicValue(bcTmp[1]));

		}
		//ȡ4�������������Ϊ10 Ȼ�����������ŵ���Ͽ��Ƿ�����������
		for ( n= 1;n<cbCardCount;n++)
		{
			for (int j = 1;j<cbCardCount;j++)
			{
				if(j != n)
				{
					if((GetCardLogicValue(cbCardDataSort[n])+GetCardLogicValue(cbCardDataSort[j]))%10==0)
					{
						int add = 0;
						for (int y = 1;y<cbCardCount;y++)
						{
							if(y != n&&y!=j)
							{
								iSingleA[add] =cbCardDataSort[y]; 
								add++;

							}

						}
						if(iBigValue<=(GetCardLogicValue(iSingleA[0])+GetCardLogicValue(iSingleA[1]))%10)
						{
							iBigValue = GetCardLogicValue(iSingleA[0])+GetCardLogicValue(iSingleA[1])%10;
							bcMakeMax[0]= cbCardDataSort[0];
							bcMakeMax[1]= cbCardDataSort[j];
							bcMakeMax[2]= cbCardDataSort[n]; 
							bcMakeMax[3]= iSingleA[0]; 
							bcMakeMax[4]= iSingleA[1]; 

							if(bcOutCadData != NULL)
							{
								memcpy(bcOutCadData,bcMakeMax,cbCardCount);
							}
							if(iBigValue==0)
							{
								return CT_SPECIAL_NIUNIU;								
							}
						}

					}
				}
			}
		}
		if(iBigValue!= 0)
		{
			return RetType(iBigValue);
		}else
		{
			//�����ϲ��ɹ�
			iGetTenCount = 0;
		}

	}
	if(iGetTenCount==0)
	{
		//5�����
		for ( n= 0;n<cbCardCount;n++)
		{
			for (int j = 0;j<cbCardCount;j++)
			{
				if(j != n)
				{
					for (int w = 0;w<cbCardCount;w++)
					{
						if(w != n&&w!=j)
						{
							int valueAdd = GetCardLogicValue(cbCardDataSort[n]);
							valueAdd += GetCardLogicValue(cbCardDataSort[j]);
							valueAdd += GetCardLogicValue(cbCardDataSort[w]);

							if(valueAdd%10==0)
							{
								int add = 0;
								for (int y = 0;y<cbCardCount;y++)
								{
									if(y != n&&y!=j&&y!=w)
									{
										iSingleA[add] =cbCardDataSort[y]; 
										add++;

									}

								}
								if(iBigValue<=(GetCardLogicValue(iSingleA[0])+GetCardLogicValue(iSingleA[1]))%10)
								{
									iBigValue = GetCardLogicValue(iSingleA[0])+GetCardLogicValue(iSingleA[1])%10;
									bcMakeMax[0]= cbCardDataSort[n];
									bcMakeMax[1]= cbCardDataSort[j];
									bcMakeMax[2]= cbCardDataSort[w]; 
									bcMakeMax[3]= iSingleA[0]; 
									bcMakeMax[4]= iSingleA[1]; 

									if(bcOutCadData != NULL)
									{
										memcpy(bcOutCadData,bcMakeMax,cbCardCount);
									}
									if(iBigValue==0)
									{
										return CT_SPECIAL_NIUNIU;                                        
									}
								}

							}

						}
					}
				}
			}		
		}
		if(iBigValue!=0)
		{
			return RetType(iBigValue);
		}	
		else
		{
			return CT_POINT;
		}
	}
	return CT_POINT;
}

//��С�Ƚ�
/*
cbNextCardData>cbFirstCardData  ����1
cbNextCardData<cbFirstCardData  ����-1
cbNextCardData==cbFirstCardData ����0
*/
//Multiple �Ƚϳ����ı���
int CNiuNiuLogic::CompareCard(const BYTE cbFirstCardData[], BYTE cbFirstCardCount,const BYTE cbNextCardData[], BYTE cbNextCardCount,BYTE &Multiple,BYTE multipleType, tagNiuMultiple * ptagNiuMultiple)
{
	//�Ϸ��ж�
	ASSERT(5==cbFirstCardCount && 5==cbNextCardCount);
	if (!(5==cbFirstCardCount && 5==cbNextCardCount)) return 0;

	int32 s_Multiple[CT_SPECIAL_BOMEBOME+1];
	if(multipleType == 0) {//��ţ
		int32 s_Multiple1[] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 3, 3};
		if (ptagNiuMultiple != NULL) {
			s_Multiple1[CT_SPECIAL_NIU1] = ptagNiuMultiple->niu1;
			s_Multiple1[CT_SPECIAL_NIU2] = ptagNiuMultiple->niu2;
			s_Multiple1[CT_SPECIAL_NIU3] = ptagNiuMultiple->niu3;
			s_Multiple1[CT_SPECIAL_NIU4] = ptagNiuMultiple->niu4;
			s_Multiple1[CT_SPECIAL_NIU5] = ptagNiuMultiple->niu5;
			s_Multiple1[CT_SPECIAL_NIU6] = ptagNiuMultiple->niu6;
			s_Multiple1[CT_SPECIAL_NIU7] = ptagNiuMultiple->niu7;
			s_Multiple1[CT_SPECIAL_NIU8] = ptagNiuMultiple->niu8;
			s_Multiple1[CT_SPECIAL_NIU9] = ptagNiuMultiple->niu9;
			s_Multiple1[CT_SPECIAL_NIUNIU] = ptagNiuMultiple->niuniu;
			s_Multiple1[CT_SPECIAL_NIUNIUDW] = ptagNiuMultiple->big5niu;
			s_Multiple1[CT_SPECIAL_NIUNIUXW] = ptagNiuMultiple->small5niu;
			s_Multiple1[CT_SPECIAL_BOMEBOME] = ptagNiuMultiple->bomebome;
		}
		memcpy(s_Multiple,s_Multiple1,sizeof(s_Multiple));
	}else if(multipleType == 1){//ͨ��ţţ
		int32 s_Multiple1[] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 3, 3};
		memcpy(s_Multiple,s_Multiple1,sizeof(s_Multiple));
	}else{//��ͨţţ
		int32 s_Multiple1[] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 3, 3};
		memcpy(s_Multiple,s_Multiple1,sizeof(s_Multiple));
	}

	//��ȡ����
	BYTE cbFirstCardType=GetCardType(cbFirstCardData, cbFirstCardCount);
	BYTE cbNextCardType=GetCardType(cbNextCardData, cbNextCardCount);

    Multiple = s_Multiple[cbFirstCardType];
	//���ͱȽ�
	if(cbFirstCardType != cbNextCardType) 
	{
		if(cbNextCardType > cbFirstCardType)
		{
            Multiple = s_Multiple[cbNextCardType];
			return 1;
		}
		else
		{
            Multiple = s_Multiple[cbFirstCardType];
			return -1;
		}
	}

	//���������ж�
	if(CT_POINT != cbFirstCardType && cbFirstCardType == cbNextCardType)
	{
		//�����˿�
		BYTE cbFirstCardDataTmp[CARD_COUNT], cbNextCardDataTmp[CARD_COUNT];

		memcpy(cbFirstCardDataTmp,cbFirstCardData,sizeof(BYTE)*cbFirstCardCount);
		memcpy(cbNextCardDataTmp,cbNextCardData,sizeof(BYTE)*cbNextCardCount);
		SortCardList(cbFirstCardDataTmp,cbFirstCardCount,ST_NEW);
		SortCardList(cbNextCardDataTmp,cbNextCardCount,ST_NEW);

        Multiple = s_Multiple[cbFirstCardType];
            
		BYTE firstValue = GetCardNewValue(cbFirstCardDataTmp[0]);
		BYTE NextValue = GetCardNewValue(cbNextCardDataTmp[0]);

		BYTE firstColor = GetCardColor(cbFirstCardDataTmp[0]);
		BYTE NextColor = GetCardColor(cbNextCardDataTmp[0]);
		if(firstValue<NextValue)
		{
			return 1;
		}else
		{
			if(firstValue==NextValue)
			{
				if(firstColor<NextColor)
				{
					return 1;

				}else
				{
					return -1;
				}
			}
			return -1;
		}
	}

	//�����˿�
	BYTE cbFirstCardDataTmp[CARD_COUNT], cbNextCardDataTmp[CARD_COUNT];
	memcpy(cbFirstCardDataTmp,cbFirstCardData,sizeof(BYTE)*cbFirstCardCount);
	memcpy(cbNextCardDataTmp,cbNextCardData,sizeof(BYTE)*cbNextCardCount);
	SortCardList(cbFirstCardDataTmp,cbFirstCardCount,ST_NEW);
	SortCardList(cbNextCardDataTmp,cbNextCardCount,ST_NEW);

	BYTE firstValue = GetCardNewValue(cbFirstCardDataTmp[0]);
	BYTE NextValue = GetCardNewValue(cbNextCardDataTmp[0]);
	BYTE firstColor = GetCardColor(cbFirstCardDataTmp[0]);
	BYTE NextColor = GetCardColor(cbNextCardDataTmp[0]);

	if(firstValue<NextValue)
	{
		return 1;
	}else
	{
		if(firstValue==NextValue)
		{
			if(firstColor<NextColor)
			{
				return 1;

			}else
			{
				return -1;
			}
		}
		return -1;
	}
}

//��ȡ�Ƶ�
BYTE CNiuNiuLogic::GetCardListPip(const BYTE cbCardData[], BYTE cbCardCount)
{
	//��������
	BYTE cbPipCount=0;

	//��ȡ�Ƶ�
	BYTE cbCardValue=0;
	for (BYTE i=0;i<cbCardCount;i++)
	{
		cbCardValue=GetCardValue(cbCardData[i]);
		if(cbCardValue>10)
		{
			cbCardValue = 10;

		}
		cbPipCount+=cbCardValue;
	}
	return (cbPipCount%10);
}
BYTE CNiuNiuLogic::GetCardNewValue(BYTE cbCardData)
{
	//�˿�����
	BYTE cbCardColor=GetCardColor(cbCardData);
	BYTE cbCardValue=GetCardValue(cbCardData);

	//ת����ֵ
	if (cbCardColor==0x04) return cbCardValue+13+2;
	return cbCardValue;

}
//�߼���С
BYTE CNiuNiuLogic::GetCardLogicValue(BYTE cbCardData)
{
	BYTE cbValue=GetCardValue(cbCardData);

	//��ȡ��ɫ
	BYTE cbColor=GetCardColor(cbCardData);

	if(cbValue>10)
	{
		cbValue = 10;
	}
	if(cbColor==0x4)
	{
		return 11;
	}
	return cbValue;
}

//�����˿�
void CNiuNiuLogic::SortCardList(BYTE cbCardData[], BYTE cbCardCount, BYTE cbSortType)
{
	//��Ŀ����
	if (cbCardCount==0) return;

	//ת����ֵ
	BYTE cbSortValue[CARD_COUNT];
	if (ST_VALUE==cbSortType)
	{
		for (BYTE i=0;i<cbCardCount;i++) cbSortValue[i]=GetCardValue(cbCardData[i]);	
	}
	else 
	{
		if(cbSortType==ST_NEW)
		{
			for (BYTE i=0;i<cbCardCount;i++) cbSortValue[i]=GetCardNewValue(cbCardData[i]);	

		}else
		{
			for (BYTE i=0;i<cbCardCount;i++) cbSortValue[i]=GetCardLogicValue(cbCardData[i]);	

		}

	}


	//�������
	bool bSorted=true;
	BYTE cbThreeCount,cbLast=cbCardCount-1;
	do
	{
		bSorted=true;
		for (BYTE i=0;i<cbLast;i++)
		{
			if ((cbSortValue[i]<cbSortValue[i+1])||
				((cbSortValue[i]==cbSortValue[i+1])&&(cbCardData[i]<cbCardData[i+1])))
			{
				//����λ��
				cbThreeCount=cbCardData[i];
				cbCardData[i]=cbCardData[i+1];
				cbCardData[i+1]=cbThreeCount;
				cbThreeCount=cbSortValue[i];
				cbSortValue[i]=cbSortValue[i+1];
				cbSortValue[i+1]=cbThreeCount;
				bSorted=false;
			}	
		}
		cbLast--;
	} while(bSorted==false);

	return;
}

bool CNiuNiuLogic::GetSubDataCard(BYTE cbSubCardData[][NIUNIU_CARD_NUM], vector<BYTE> & vecRemainCardData)
{
	vecRemainCardData.clear();
	BYTE cbCardListData[CARD_COUNT] = { 0 };
	memcpy(cbCardListData, m_cbCardListData, sizeof(cbCardListData));

	//��������
	//BYTE cbRandCount = 0, cbPosition = 0;
	//do
	//{
	//	cbPosition = g_RandGen.RandRange(0, CARD_COUNT - cbRandCount - 1);
	//	BYTE cbTempSwapCardData = cbCardListData[cbPosition];
	//	cbCardListData[cbPosition] = cbCardListData[CARD_COUNT - cbRandCount - 1];
	//	cbCardListData[CARD_COUNT - cbRandCount - 1] = cbTempSwapCardData;

	//	cbRandCount++;
	//} while (cbRandCount<CARD_COUNT);


	for (int i = 0; i < CARD_COUNT; i++)
	{
		BYTE cbTempCardData = cbCardListData[i];
		bool bIsInSubCard = false;
		for (int i = 0; i < NIUNIU_CARD_NUM; i++)
		{
			for (int j = 0; j < NIUNIU_CARD_NUM; j++)
			{
				if (cbSubCardData[i][j] == cbTempCardData)
				{
					bIsInSubCard = true;
					break;
				}
			}
			if (bIsInSubCard)
			{
				break;
			}
		}
		if (bIsInSubCard == false)
		{
			vecRemainCardData.push_back(cbCardListData[i]);
		}
	}

	return true;
}
//��Ч�ж�
bool CNiuNiuLogic::IsValidCard(BYTE cbCardData)
{
	for (int i = 0; i < CARD_COUNT; i++)
	{
		if (m_cbCardListData[i] == cbCardData)
		{
			return true;
		}
	}
	return false;
}

};

