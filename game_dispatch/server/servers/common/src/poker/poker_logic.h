//
// Created by toney on 16/4/19.
//

#ifndef SERVER_POKER_LOGIC_H
#define SERVER_POKER_LOGIC_H

#include "svrlib.h"

// �˿���ֵ����
#define	MASK_COLOR	    0xF0	//��ɫ����
#define	MASK_VALUE	    0x0F	//��ֵ����

#define FULL_POKER_COUNT  54    // һ���˿�����Ŀ
#define INVALID_CHAIR     0xFF  //


class CPokerLogic
{
public:
    CPokerLogic();
    ~CPokerLogic();

    //��ȡ�˿���ֵ
    virtual uint8    GetCardValue(uint8 cbCardData) { return cbCardData&MASK_VALUE; }
    //��ȡ�˿˻�ɫ
    virtual uint8    GetCardColor(uint8 cbCardData) { return cbCardData&MASK_COLOR; }
    //��ȡ�˿˻�ɫֵ
    virtual uint8    GetCardColorValue(uint8 cbCardData) { return (cbCardData&MASK_COLOR)>>4; }

    //��Ч�ж�
    virtual bool     IsValidCard(BYTE cbCardData);
    //�����˿�
    virtual BYTE     MakeCardData(BYTE cbValueIndex, BYTE cbColorIndex);

    // �������(ע�������Ͻ��ֵ����)
    void    Combination(const vector<BYTE>& t,int c,vector< vector<BYTE> >& results);
    void    PrintCombRes(const vector<BYTE>& t,vector<int>& vecInt,vector< vector<BYTE> >& results);
    void    SubVector(const vector<BYTE>& t,const vector<BYTE>& ex,vector<BYTE>& res);

    static  bool compare(BYTE a, BYTE b);


};
















#endif //SERVER_POKER_LOGIC_H
