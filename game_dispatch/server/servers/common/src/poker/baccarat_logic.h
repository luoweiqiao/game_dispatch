//
// Created by toney on 16/8/27.
//

#ifndef SERVER_BACCARAT_LOGIC_H
#define SERVER_BACCARAT_LOGIC_H

#include "svrlib.h"
#include "poker_logic.h"

namespace game_baccarat
{
    //��Ϸ�߼�
    class CBaccaratLogic : public CPokerLogic
    {
        //��������
    private:
        static const BYTE m_cbCardListData[52 * 8];                //�˿˶���

        //��������
    public:
        //���캯��
        CBaccaratLogic();

        //��������
        virtual ~CBaccaratLogic();
    public:
        //�����˿�
        void RandCardList(BYTE cbCardBuffer[], int32 cbBufferCount);

        //�߼�����
    public:
        //��ȡ�Ƶ�
        BYTE GetCardPip(BYTE cbCardData);

        //��ȡ�Ƶ�
        BYTE GetCardListPip(const BYTE cbCardData[], BYTE cbCardCount);
    };

};

#endif //SERVER_BACCARAT_LOGIC_H
