#include<time.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<pthread.h>

#define MAXLINE 512
#define MAXMEM 8
#define NAMELEN 20
#define SERV_PORT 8080
#define LISTENQ 5

int listenfd,connfd[MAXMEM];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char user[MAXMEM][NAMELEN];
void Quit();
void rcv_snd(int n);

typedef enum setOX
{
    OX_blank = 0,
    OX_O = 1,
    OX_X = 2,
}OX;

void Print(OX array[][3]) //印出OX棋
{
    printf("\n");
    for(int i=0; i<3; i++)
    {
        for(int j=0; j<3; j++)
        {
            if(array[i][j] == OX_blank)
                printf("#");
            else if(array[i][j] == OX_O)
                printf("O");
            else
                printf("X");
        }
        printf("\n");
    }
    printf("\n");
}

OX CheckWin(OX array[][3]) //檢查是否連線
{
    for(int i=0; i<3; i++) //橫列
        if(array[i][0] != OX_blank)
        if(array[i][0] == array[i][1] && array[i][0] == array[i][2])
            return array[i][0];
    for(int j=0; j<3;j++) //直列
        if(array[0][j] != OX_blank)
        if(array[0][j] == array[1][j] && array[0][j]== array[2][j])
            return array[0][j];
    //左上右下
    if(array[0][0] == array[1][1] && array[0][0] == array[2][2] && array[0][0] != OX_blank)
        return array[0][0];
    //右上左下
    if(array[0][2] == array[1][1] && array[0][2] == array[2][0] && array[0][2] != OX_blank)
        return array[0][2];

    return OX_blank;
}

int main()
{
    pthread_t thread;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t length;
    char buff[MAXLINE];

    //用socket建server的fd
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if(listenfd < 0)
    {
        printf("Socket created failed.\n");
        return -1;
    }
    //網路連線設定
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);	//port80
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //用bind開監聽器
    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Bind failed.\n");
        return -1;
    }
    //用listen開始監聽
    printf("listening...\n");
    listen(listenfd, LISTENQ);
    //建立thread管理server
    pthread_create(&thread, NULL, (void*)(&Quit), NULL);
    //紀錄閒置的client(-1)
    //initialize
    int i=0;
    for(i=0; i<MAXMEM; i++)
    {
        connfd[i]=-1;
    }
    memset(user, '\0', sizeof(user));
    printf("initialize...\n");

    while(1)
    {
        length = sizeof(cli_addr);
        for(i=0; i<MAXMEM; i++)
            if(connfd[i]==-1) break;
        //等待client連線
        printf("receiving...\n");
        connfd[i] = accept(listenfd, (struct sockaddr*)&cli_addr, &length);
        //對新client建thread，以開啟訊息處理
        pthread_create(malloc(sizeof(pthread_t)), NULL, (void*)(&rcv_snd), (void*)i);
    }

    return 0;
}
//關閉server
void Quit()
{
    char msg[10];
    while(1)
    {
        scanf("%s", msg);
        if(strcmp("/quit",msg)==0)
        {
            printf("結束\n");
            close(listenfd);
            exit(0);
        }
    }
}

void rcv_snd(int n)
{
    char msg_notify[MAXLINE];
    char msg_recv[MAXLINE];
    char msg_send[MAXLINE];
    char who[MAXLINE];
    char name[NAMELEN];
    char message[MAXLINE];

    char choose_rival[]="請選擇對手\n";
    char check[MAXLINE];
    char ok[3];

    int i=0;
    int retval;

    //get client name
    int length;
    length = recv(connfd[n], name, NAMELEN, 0);
    if(length>0)
    {
        name[length] = 0;
        strcpy(user[n], name);
    }
    //通知有新client
    memset(msg_notify, '\0', sizeof(msg_notify));
    strcpy(msg_notify, name);
    strcat(msg_notify, " join\n");
    for(i=0; i<MAXMEM; i++)
        if(connfd[i]!=-1) send(connfd[i], msg_notify, strlen(msg_notify), 0);
    //接收某client的訊息並轉發
    while(1)
    {
        memset(msg_recv, '\0', sizeof(msg_recv));
        memset(msg_send, '\0', sizeof(msg_send));
        memset(message,'\0',sizeof(message));
        memset(check,'\0',sizeof(check));

        if((length=recv(connfd[n], msg_recv, MAXLINE, 0))>0)
        {
            msg_recv[length]=0;
            //輸入quit離開
            if(strcmp("/quit", msg_recv)==0)
            {
                close(connfd[n]);
                connfd[n]=-1;
                pthread_exit(&retval);
            }
            else if(strncmp("/game", msg_recv, 5)==0)
            {
                int p1,p2;
                send(connfd[n], choose_rival, strlen(choose_rival), 0); //詢問選擇對手
                length = recv(connfd[n], who, MAXLINE, 0);
                who[length]=0;
                printf("遊戲開始\n");
                printf("請輸入座標(0 0)~(2 2)\n");
                strcpy(msg_send,"遊戲開始\n");
                for(i=0; i<MAXMEM; i++)
                {
                    if(connfd[i]!=-1)
                    {
                        if(strncmp(who, user[i], strlen(who)-1)==0)
                        {
                            send(connfd[i], msg_send, strlen(msg_send), 0);
                            p2=i;
                        }
                        else if(strncmp(name, user[i], strlen(name)-1)==0)
                        {
                            send(connfd[i], msg_send, strlen(msg_send), 0);
                            p1=i;
                        }
                    }
                }
                strcpy(msg_send,"請輸入座標(0 0)~(2 2)\n");
                send(connfd[p1], msg_send, strlen(msg_send), 0);
                send(connfd[p2], msg_send, strlen(msg_send), 0);
                OX array[3][3] = {OX_blank};
                int counter = 0;
                Print(array);
                int flag=1,num_x,num_y;
                while(1)
                {
                    int pos_check=1;
                    if(flag==1)
                    {
                        strcpy(msg_send,"輪到你了：\n");
                        send(connfd[p1], msg_send, strlen(msg_send), 0);
                        length = recv(connfd[p1], message, MAXLINE, 0);
                        message[length]=0;
                        strcpy(msg_send, user[p1]);
                        strcat(msg_send, ": ");
                        strcat(msg_send, message);
                        send(connfd[p2], msg_send, strlen(msg_send), 0);
                        num_x=atoi(message);
                        num_y=atoi(message+2);
                        if(! ((num_x >= 0 && num_x < 3)&&(num_y >= 0 && num_y < 3)) || (array[num_x][num_y] != OX_blank))
                        {
                            printf("輸入位置錯誤！\n");
                            strcpy(msg_send,"輸入位置錯誤！\n");
                            send(connfd[p1], msg_send, strlen(msg_send), 0);
                            pos_check=0;
                            continue;
                        }
                        printf("flag:%d num1,num2:%d %d\n",flag,num_x, num_y);
                        flag=2;
                        //no exception solve
                    }
                    else if(flag==2)
                    {
                        strcpy(msg_send,"輪到你了：\n");
                        send(connfd[p2], msg_send, strlen(msg_send), 0);
                        length = recv(connfd[p2], message, MAXLINE, 0);
                        message[length]=0;
                        strcpy(msg_send, user[p2]);
                        strcat(msg_send, ": ");
                        strcat(msg_send, message);
                        send(connfd[p1], msg_send, strlen(msg_send), 0);
                        num_x=atoi(message);
                        num_y=atoi(message+2);
                        if(! ((num_x >= 0 && num_x < 3)&&(num_y >= 0 && num_y < 3)) || (array[num_x][num_y] != OX_blank))
                        {
                            printf("輸入位置錯誤！\n");
                            strcpy(msg_send,"輸入位置錯誤！\n");
                            send(connfd[p1], msg_send, strlen(msg_send), 0);
                            pos_check=0;
                            continue;
                        }
                        printf("flag:%d num1,num2:%d %d\n",flag,num_x,num_y);
                        flag=1;
                    }

                    array[num_x][num_y] = counter % 2 ? OX_O : OX_X;
                    Print(array);
                    if(OX_O == CheckWin(array))
                    {
                        strcpy(msg_send,user[p2]);
                        strcat(msg_send," 勝利\n");
                        send(connfd[p2], msg_send, strlen(msg_send), 0);
                        send(connfd[p1], msg_send, strlen(msg_send), 0);
                        printf("遊戲結束, O贏了\n");
                        break;
                    }
                    else if(OX_X == CheckWin(array))
                    {
                        strcpy(msg_send,user[p1]);
                        strcat(msg_send," 勝利\n");
                        send(connfd[p2], msg_send, strlen(msg_send), 0);
                        send(connfd[p1], msg_send, strlen(msg_send), 0);
                        printf("遊戲結束, X贏了\n");
                        break;
                    }
                    if (pos_check==1) counter++;
                    if(counter>=9)
                    {
                        strcpy(msg_send,"平手\n");
                        send(connfd[p1], msg_send, strlen(msg_send), 0);
                        send(connfd[p2], msg_send, strlen(msg_send), 0);
                        printf("平手\n");
                        break;
                    }
                }
            }
            //顯示使用者
            else if(strncmp("/userlist", msg_recv, 5)==0)
            {
                strcpy(msg_send, "<SERVER> Online:");
                for(i=0; i<MAXMEM; i++) {
                    if(connfd[i]!=-1) {
                        strcat(msg_send, user[i]);
                        strcat(msg_send, " ");
                    }
                }
                strcat(msg_send, "\n");
                send(connfd[n], msg_send, strlen(msg_send), 0);
            }
            //傳給每個人
            else
            {
                strcpy(msg_send, name);
                strcat(msg_send,": ");
                strcat(msg_send, msg_recv);

                for(i=0;i<MAXMEM;i++)
                {
                    if(connfd[i]!=-1)
                    {
                        if(strcmp(name, user[i])==0) continue;
                        else send(connfd[i], msg_send, strlen(msg_send), 0);
                    }
                }
            }
        }
    }
}
