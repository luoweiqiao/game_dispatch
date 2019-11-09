//
// Created by toney on 16/8/27.
//

#ifndef SERVER_PAIJIU_LOGIC_H
#define SERVER_PAIJIU_LOGIC_H

#include "svrlib.h"
#include "poker_logic.h"

namespace game_paijiu
{

    //�˿�����
    enum emPAIJIU_CARD_TYPE
    {
        CT_ERROR					= 0,									//��������
        CT_POINT_0					= 29,									//��ʮ
        CT_POINT_1					= 28,									//һ��
        CT_POINT_2					= 27,									//����
        CT_POINT_3					= 26,									//����
        CT_POINT_4					= 25,									//�ĵ�
        CT_POINT_5					= 24,									//���
        CT_POINT_6					= 23,									//����
        CT_POINT_7					= 22,									//�ߵ�
        CT_POINT_8					= 21,									//�˵�
        CT_POINT_9					= 20,									//�ŵ�
        CT_SPECIAL_19				= 19,									//�ظ�
        CT_SPECIAL_18				= 18,									//���
        CT_SPECIAL_17				= 17,									//�����
        CT_SPECIAL_16				= 16,									//����
        CT_SPECIAL_15				= 15,									//����
        CT_SPECIAL_14				= 14,									//�Ӱ�
        CT_SPECIAL_13				= 13,									//�Ӿ�
        CT_SPECIAL_12				= 12,									//˫����
        CT_SPECIAL_11				= 11,									//˫ͭ��
        CT_SPECIAL_10				= 10,									//˫��ͷ
        CT_SPECIAL_9				= 9,								    //˫��ͷ
        CT_SPECIAL_8				= 8,									//˫���
        CT_SPECIAL_7				= 7,									//˫��
        CT_SPECIAL_6				= 6,									//˫÷
        CT_SPECIAL_5				= 5,									//˫��
        CT_SPECIAL_4				= 4,									//˫��
        CT_SPECIAL_3				= 3,									//˫��
        CT_SPECIAL_2				= 2,									//˫��
        CT_SPECIAL_1				= 1,									//����
    };

    //��������
    const static int32 	ST_VALUE = 1;									//��ֵ����
    const static int32	ST_LOGIC = 2;									//�߼�����
    //�˿���Ŀ
    const static  int32 CARD_COUNT = 32;								//�˿���Ŀ
    //��ֵ����
    const static BYTE LOGIC_MASK_COLOR = 0xF0;							//��ɫ����
    const static BYTE LOGIC_MASK_VALUE = 0x0F;							//��ֵ����

    //��Ϸ�߼�
    class CPaijiuLogic : public CPokerLogic
    {
        //��������
    public:
        static const BYTE	m_cbCardListData[CARD_COUNT];		//�˿˶���

        //��������
    public:
        //���캯��
        CPaijiuLogic();

        //��������
        virtual ~CPaijiuLogic();
    public:
        //��ȡ��ֵ
        BYTE GetCardValue(BYTE cbCardData) { return cbCardData&LOGIC_MASK_VALUE; }
        //��ȡ��ɫ
        BYTE GetCardColor(BYTE cbCardData) { return (cbCardData&LOGIC_MASK_COLOR)>>4; }

        //�����˿�
        void RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount);
        //�����˿�
        void SortCardList(BYTE cbCardData[], BYTE cbCardCount, BYTE cbSortType);

        //�߼�����
    public:
        //��ȡ�Ƶ�
        BYTE GetCardListPip(const BYTE cbCardData[], BYTE cbCardCount);
        //��ȡ����
        BYTE GetCardType(const BYTE cbCardData[], BYTE cbCardCount);
        //��С�Ƚ�
        int CompareCard(const BYTE cbFirstCardData[], BYTE cbFirstCardCount,const BYTE cbNextCardData[], BYTE cbNextCardCount);
        //�߼���С
        BYTE GetCardLogicValue(BYTE cbCardData);





    };

};

#endif //SERVER_BACCARAT_LOGIC_H
