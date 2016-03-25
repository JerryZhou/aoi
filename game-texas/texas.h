#include "aoi.h"

typedef struct itexas_pot {
    int next;
}ipot;

/* 玩家状态 */
typedef enum EnumTexasPlayerState {
    EnumTexasPlayerState_Waiting,
    EnumTexasPlayerState_AllIn,
    EnumTexasPlayerState_Burn,
}EnumTexasPlayerState;

/* 玩家角色 */
typedef enum EnumTexasPlayerRole {
    EnumTexasPlayerRole_Visitor,    /*围观群众*/
    EnumTexasPlayerRole_Big,        /*小盲注*/
    EnumTexasPlayerRole_Small,      /*大盲注*/
    EnumTexasPlayerRole_Starter,    /*庄家*/
    EnumTexasPlayerRole_Normal,     /*普通玩家*/
}EnumTexasPlayerRole;

typedef struct itexas_player {
    int playerid;
}itexas_player;

/*玩家在下一轮的选择*/
typedef enum EnumTexasTrunChoose {
    EnumTexasTrunChoose_Burn,   /*消牌*/
    EnumTexasTrunChoose_Skip,   /*跳过*/
    EnumTexasTrunChoose_AllIn,  /*soha*/
    EnumTexasTrunChoose_Append, /*加注*/
}EnumTexasTrunChoose;

typedef struct itexas_trun {
    int sessionid;
}itexas_trun;

typedef struct itexas_table {
    /* []*itexas_player */
    iarray *players;
    
    /* 对于桌子 来说，每局都分配一个 id */
    int sessionid;
}itexas_table;

