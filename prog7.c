/*
   Zachary Ikpefua
   ECE 2220
   Project 7
   Spring 2018
   Purpose:	The purpose of this program is to create a simulation that
            runs up to four processes that will similate a submarine moving
            in the water
   Assumptions:	User needs to know to open four terminals, since the program is
                sleeping for 1 second, user needs to reduce inputs. If
                program does seem to take inputs, user should kill Terminals
                and relogin to the school computers
   Known Bugs:	Doesnt display scuttle for third ship
   */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MISSIONFAILURE 23
#define MISSIONSUCCESS 42

FILE* HomeBase;

struct submarine
{
        FILE * terminal;
        int missiles;
        int subNumber;
        int checkUpdate;
        int fuel;
        int distance;
        int checkReturn; //Used to make sub return to HomeBase
} SubStruct;

void userInstructions(void);
void changeInactiveState(int subPID);
int randomizeGenerator(struct submarine sub);
int checkifSubAlive(int subPID);
void AlarmSigHandler(int signal);
void User1SigHandler(int signal);
void User2SigHandler(int signal);
char *getCurrentTime();
void checkSubmarineStatus(struct submarine* sub);
void updateSubmarine(struct submarine* sub);
void createActiveArray();
void GameOver(void);
void displaySubStats(struct submarine sub);
void DestroySigHandler(int signal);
char *createInput();
struct submarine createSubmarine(char* Buffer, int);
int checkSubmarineNum(int num);
int convertToPID(int num);

int pidofBase;
int pidSub1;
int pidSub2;
int pidSub3;
static int dontRepeat1 = 1;
static int dontRepeat2 = 1;
static int dontRepeat3 = 1;
static int notDestroyed1 = 1;
static int notDestroyed2 = 1;
static int notDestroyed3 = 1;
int missionSuccess;
int subsAmount;
int aliveSubArray[3];

int main(void){
        system("clear");
        missionSuccess = 1;
        char userCommand[10];
        char cmd1;
        int cmd2;
        int subPID;
        char* termName[4];
        int term;
        int numberOfSub = 0;
        int currentPID = -1;
        signal(SIGALRM, AlarmSigHandler);
        signal(SIGUSR1, User1SigHandler);
        signal(SIGUSR2, User2SigHandler);
        signal(SIGTERM, DestroySigHandler);


        for(term = 0; term < 4; term++)
        {
                termName[term] = createInput();
        }

        srand((int)time(NULL));

        pidofBase = getpid();
        //Set homebase to the first thing in the termName, w+ checks to see if it is there
        HomeBase = fopen(termName[0],"w+");
        subsAmount = 3;
        createActiveArray();

        pidSub1 = -1;
        pidSub2 = -1;
        pidSub3 = -1;

        fprintf(HomeBase, "The Mission has Started! Current Time: %s\n", getCurrentTime());

        //Creating the Child Proccess
        pidSub1 = fork();
        numberOfSub = 1;
        currentPID = getpid();
        if(currentPID == pidofBase)
        {
                pidSub2 = fork();
                numberOfSub=2;
        }
        currentPID = getpid();

        if(currentPID == pidofBase)
        {
                pidSub3 = fork();
                numberOfSub = 3;
        }
        currentPID = getpid();

        if(currentPID != pidofBase) SubStruct = createSubmarine(termName[numberOfSub],numberOfSub);

        while(currentPID != pidofBase) {
                alarm(1);
                sleep(1);
        }
        while(currentPID == pidofBase)
        {
                fgets(userCommand, 10, HomeBase);
                sscanf(userCommand, "%c%d", &cmd1,&cmd2);

                if(subsAmount < 1)
                {
                        GameOver();
                }
                //Convert cmd2 to respective sub
                subPID = convertToPID(cmd2);
                //Launching a missile
                fflush(HomeBase);
                if(cmd1 == 'l') {
                        if(cmd2 < 1 || cmd2 > 3) { //cmd2 needs to be 1-3 for eveyr process
                                userInstructions();
                        }
                        if(checkSubmarineNum(cmd2) && checkifSubAlive(subPID))
                        {
                                kill(subPID, SIGUSR1);
                        }
                }
                //Refueling a seleced sub
                else if(cmd1 == 'r') {
                        if(cmd2 < 1 || cmd2 > 3) {
                                userInstructions();
                        }
                        if(checkSubmarineNum(cmd2) && checkifSubAlive(subPID))
                        {
                                kill(subPID, SIGUSR2);
                        }

                }
                //Destroying a submarine
                else if(cmd1 == 's') {
                        if(cmd2 < 1 || cmd2 > 3) {
                                userInstructions();
                        }
                        if(checkSubmarineNum(cmd2) && checkifSubAlive(subPID))
                        {
                                kill(convertToPID(cmd2), SIGTERM );
                                missionSuccess = 0;
                                changeInactiveState(subPID);
                                subsAmount--;
                        }
                        if(subsAmount == 0)
                        {
                                GameOver();
                        }
                }
                //Quiting the program
                else if(cmd1 == 'q') {
                        missionSuccess = 1;
                        GameOver();
                }
                //Input is incorrect
                else{
                        userInstructions();
                }

        }
        //this never runs because quit runs kill()
        return 0;
}

/*
   Function: userInstructions
   Input: none
   Ouputs: none
   Purpose: Display to the user instructions if they type an incorrect command
 */
void userInstructions(void) {
        printf("\aError: Invalid command. Below are valid command inputs\n");
        printf("ln – Orders submarine n (n = 1 to 3) to launch a missile.\n");
        printf("rn – Refuels submarine n’s fuel tank to 5000 gallons\n");
        printf("sn – Scuttles submarine n, i.e. kills process for submarine n.\n");
        printf("q – Exits the program.\n");
}

/*
   Function: gameOver
   Input: none
   Outpus: none
   Purpose: Kill all of the child proccesses and parent
        proccesses and end the simulation
 */
void GameOver(void){
        time_t currTime;
        time(&currTime);
        kill(pidSub2, SIGKILL);
        kill(pidSub3, SIGKILL);
        kill(pidSub1, SIGKILL);
        if(missionSuccess == 1) {
                fprintf(HomeBase,"Mission was a great success! Great job everybody!\n");
                fprintf(HomeBase,"Mission Has Ended, Time is: %s", ctime(&currTime));
                fflush(HomeBase);
                exit(MISSIONSUCCESS);
        }
        else{
                fprintf(HomeBase,"Mission Failed, We'll Get Em' Next Time\n");
                fprintf(HomeBase,"All submarines have to make it back for a Mission to be Successful!\n");
                exit(MISSIONFAILURE);
        }

}
// Generates a random values that the children can use.
/*
   Function: randomizeGenerator
   Inputs: struct Submarine
   Output: seed numbers
   Purpose: Used in order to seed the srand function for unique numbers
 */
int randomizeGenerator(struct submarine sub)
{
        return (int)(time(0)^sub.subNumber);
}


// Prints the information for a submarine.
/*
   Function: displaySubStats
   Inputs: struct Submarine
   Output: none
   Purpose: Print out the information out to the terminal
 */
void displaySubStats(struct submarine sub)
{

        fprintf(sub.terminal,"Time: %s\n", getCurrentTime());
        fprintf(sub.terminal,"\tSub %d to base, %d fuel left, %d missiles left,\t%d miles from base.\n",sub.subNumber, sub.fuel, sub.missiles, sub.distance);
        fflush(sub.terminal);
}


/*
   Function checkSubmarineStatus
   Inputs: Struct Submarine
   Ouputs: none
   Purpose: Check and see how the submarine is doing, its status
 */
void checkSubmarineStatus(struct submarine* sub)
{
        //if the sub runs out of fuel, mission is failed
        if( (sub->distance > 0) && (sub->fuel <=0) ) {
                fprintf(sub->terminal, "Sub %d dead in the water.\n",sub->subNumber);
                kill(pidofBase, SIGUSR2);
                exit(MISSIONFAILURE);
        }

        //If the sub makes it back to base, tell user what happened
        if(sub->checkReturn == 1 && sub->distance <=0 ) {
                fprintf(sub->terminal, "Back At Base, Awaiting Debrief.\n");
                kill(pidofBase, SIGUSR1);
                exit(MISSIONSUCCESS);
        }

        //if the sub is running out of fuel
        if( sub->fuel < 500){
                fprintf(sub->terminal, "Sub %d running out of fuel!\n",sub->subNumber);
              }
}


// This function makes all the necessary time changes to the F.
/*
   Function: updateSubmarine
   Input: struct Submarine
   Outputs: none
   Purpose: Check for the update of the submarine
 */
void updateSubmarine(struct submarine* sub)
{
        srand(randomizeGenerator(*sub));
        sub->fuel -= (rand()%101 + 100);
        if((sub->checkUpdate) % 2 == 0) {
          sub->distance += (sub->checkReturn) ? -(rand() % 6 + 3) : (rand() % 6 + 5);
        }
        checkSubmarineStatus(sub);
        //Dispaly information every three seconds
        if((sub->checkUpdate) % 3 == 0) {
                displaySubStats(*sub);
        }
        (sub->checkUpdate)++;

}


/*
   Function: createInput
   Inputs: none
   Ouputs: Name for buffer
   Purpose: Open up every possible terminals to display sub info
 */
char *createInput(void){

        //Starts at 0 the first time this function is called.
        static int ptsNumber = 0;
        char* ptsName = "/dev/pts/";
        int increase = 1;
        //Calloc to see if the terminal number has enough space
        ptsName = (char *) calloc((strlen(ptsName) + increase), sizeof(char) );
        sprintf(ptsName, "%s%d", "/dev/pts/", ptsNumber);
        FILE* fp = fopen(ptsName,"w");

        while(!fp) {
                ptsNumber++;
                sprintf( ptsName, "%s%d", "/dev/pts/", ptsNumber);
                fp = fopen( ptsName, "w");

        }
        //Increments ptsNumber so it goes to next terminal
        ptsNumber++;

        //Closes file poiner for space
        fclose(fp);
        return ptsName;
}
/*
   Function: createActiveArray
   Inputs: none
   Ouputs: none
   Purpose: Create an array that says each sub is active and not dead in water
 */
void createActiveArray(void){
        int i;
        for(i=0; i<3; i++) {
                aliveSubArray[i] = 1;
        }
}

/*
   Function: getCurrentTime
   Inputs: none
   Outputs: none
   Purpose: Get the current Time in hours, mins, seconds
 */
char *getCurrentTime(void){
        time_t currTime;
        time(&currTime);
        char* time = ctime(&currTime);
        //Anything afer here has unnessesary stuff
        time[19] ='\0';
        return &time[11];

}
/*
   Function: checkifSubAlive
   Inputs: pid of the child subs
   Ouputs: the array value of each subs
   Purpose: Checks to see if the sub is active, 1 is alive 0 is dead
 */
int checkifSubAlive(int subPID){

        if(subPID == pidSub1)
                return aliveSubArray[0];
        else if(subPID == pidSub2)
                return aliveSubArray[1];
        else if(subPID == pidSub3)
                return aliveSubArray[2];

        return 0;
}

/*
   Function: AlarmSigHandler
   Input: signal from main
   Output: none
   Purpose: Checks/Updates the submarine every second
 */
void AlarmSigHandler(int signal)
{
        if(getpid() != pidofBase)
                updateSubmarine(&SubStruct);
}
/*
   Function: changeInactiveState
   Inputs: sub PID
   Ouputs: none
   Purpose: Make the subs inactive if dead or scuttled
 */
void changeInactiveState(int subPID)
{
        if(subPID == pidSub1){
                aliveSubArray[0] = 0;
              }
        else if(subPID == pidSub2){
                aliveSubArray[1] = 0;
              }
        else if(subPID == pidSub3){
                aliveSubArray[2] = 0;
              }
}
/*
   Function: User1SigHandler
   Inputs: signal
   Output: none
   Purpsoe: Launch Missiles and check distance from HomeBase
 */
void User1SigHandler(int signal)
{
        if(getpid() == pidofBase) {
                int missionSuccess = 0;
                int successPID = 0;
                successPID = wait(&missionSuccess);
                subsAmount--;
                changeInactiveState(successPID);
                //DontRepeat used to prevent sucttle and distance from conflicting
                if(aliveSubArray[0] == 0 && dontRepeat1 == 1 && notDestroyed1 == 1){
                        fprintf(HomeBase,"Sub 1 has returned! Welcome home!\n");
                        dontRepeat1 = 0;
                }
                if(aliveSubArray[1] == 0 && dontRepeat2 == 1 && notDestroyed2 == 1){
                        fprintf(HomeBase,"Sub 2 has returned! Welcome home!\n");
                        dontRepeat2 = 0;
                }
                if(aliveSubArray[2] == 0 && dontRepeat3 == 1 && notDestroyed3 == 1) {
                        fprintf(HomeBase,"Sub 3 has returned! Welcome home!\n");
                        dontRepeat3 = 0;
                }
                if(subsAmount == 0)
                {
                        GameOver();
                }
        }
        else
        {
                if(SubStruct.missiles > 0) {
                        SubStruct.missiles--;
                }
                if(SubStruct.missiles > 0) {
                        fprintf(SubStruct.terminal,"Missile Launched: %d Missiles Remaining\n",SubStruct.missiles);

                }
                else if(SubStruct.missiles<=0) {
                  fprintf(SubStruct.terminal, "There are no more missiles left for this sub!!!\n");
                  SubStruct.checkReturn = 1;
                }
        }
}

/*
   Function User2SigHandler
   Inputs: signal
   Outpus: none
   Purpose: Function will send SOS signals to HomeBase and used to refue
          selected sub
 */
void User2SigHandler(int signal){
        if(getpid() == pidofBase) {
                int missionFail = 0;
                int DeadPid = 0;
                DeadPid = wait(&missionFail);
                missionSuccess = 0;
                subsAmount--;
                changeInactiveState(DeadPid);
                if(aliveSubArray[0] == 0 && dontRepeat1 == 1) {
                        fprintf(HomeBase, "Oh No! Rescue is on the way, Sub 1!\n");
                        dontRepeat1 = 0;
                }
                if(aliveSubArray[1] == 0 && dontRepeat2 == 1) {
                        fprintf(HomeBase, "Oh No! Rescue is on the way, Sub 2!\n");
                        dontRepeat2 = 0;
                }
                if(aliveSubArray[2] == 0 && dontRepeat3 == 1) {
                        fprintf(HomeBase, "Oh No! Rescue is on the way, Sub 3!\n");
                        dontRepeat3 = 0;
                }
                if(subsAmount == 0) {
                        GameOver();
                }
        }
        else
        {
                fprintf(SubStruct.terminal,"Sub's Fuel Tank has been Refueled!\n");
                SubStruct.fuel = 5000;
        }
}

/*
   Function: DestroySigHandler
   Inputs: Signals
   Outputs: none
   Purpose: Send a self-destruct signal to the selected subs
 */
void DestroySigHandler(int signal)
{
        if(getpid() != pidofBase)
        {

                fprintf(SubStruct.terminal,"Sub %d has unexpectedly exploded!...Wonder how that happened\n",SubStruct.subNumber);
                if(subsAmount == 0) {
                        GameOver();
                }
                exit(0);
        }
}

/*
   Function: convertToPID
   Inputs: cmd2
   Ouputs: pid of Subs
   Purpose: Takes cmd2 and converts it to subs 1,2, or 3
 */
int convertToPID(int num){
        if( num == 1)
        {
                return pidSub1;
        }
        if( num == 2)
        {
                return pidSub2;
        }
        if( num == 3)
        {
                return pidSub3;
        }
        else
        {
                return 0;
        }
}
/*
   Function: createSubmarine
   Input: buffer input, sub number
   Output: sturct submarines
   Purpose: Creates a new submarine struct for each 3 child subs
 */
struct submarine createSubmarine(char* Buffer,int num){
        struct submarine created;
        created.subNumber = num;

        //Random numbers are seeded for each sub.
        srand(randomizeGenerator(created));

        created.fuel = (rand() % 5001 + 1000);
        created.missiles = (rand() % 5 + 6);
        created.checkUpdate = 1;
        created.checkReturn = 0;
        created.distance = 0;
        created.terminal = fopen(Buffer,"w+");

        //prints a new line to it's terminal
        fprintf(created.terminal,"\n");
        return created;
}


/*
   Function: checkSubmarineNum
   Inputs: cmd2
   Output: 1 for active and alive , 0 for destroyed or empty
   Purpose: Checks to see if the sub has the vaild inputs
 */
int checkSubmarineNum(int num){

        if(num == 1 || num == 2 || num == 3)
        {
                return 1;
        }
        else
        {
                return 0;
        }
}
