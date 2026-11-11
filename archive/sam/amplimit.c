/*
AmpLimit.c : Picks resistors in pairs for linear voltage regulators configured for current.
Compiles ideally with GCC, likely also good with TCC. May need modification for BSD, Linux...
Created by Lostgallifreyan, alias The Doctor, alias Crow. Free to all who want it.
Thanks to Sam Goldwasser for making this available, as I have no internet server of my own.
*/

#include <stdio.h>
void GetInput(),KillZero();
char STR[8],G[2][9]={"PARALLEL","SERIES"},S1[24],S2[24];
double I,Imax=5000.0,mA,RT[192],RM[8]={0.01,0.1,1.0,10.0,100.0,1000.0,10000.0,100000.0},Z;
int RN[24]={10,11,12,13,15,16,18,20,22,24,27,30,33,36,39,43,47,51,56,62,68,75,82,91};
int S=1; //Preferred value series restriction: 0=S24, 1=S12, 2=S6. Default is 0 for 1% metal film.
int F=0,R,M,N,R1=0,R2=0,P1=0,P2=0,X,Y;

int main(int argc, char **argv){int A;
  R=192; M=8; while(M--){N=24; while(N--){RT[--R]=RM[M]*RN[N];}}                                            //Build resistor table from R24 series.
  printf("\n  A SELECTOR FOR LIMIT RESISTOR PAIRS FOR CURRENT REGULATED BY LM317 AND LM338");
  printf("\n           Default is PARALLEL network, use negative input for SERIES\n");
  printf("\n  Commandline switches:   (Not case sensitive.)");
  printf("\n     1R+   Disable resistors below 1 ohm. (WARNING: not good for LM338 >1.5A.)");
  printf("\n     E12   Use E12 series, not E24. (Trades accuracy for easier availability.)");
  A=argc; while(A--)if(!strcmp(argv[A],"1R+")||!strcmp(argv[A],"1r+"))F=23;
  A=argc; while(A--)if(!strcmp(argv[A],"E12")||!strcmp(argv[A],"e12"))S=2;
  while(1){
    I=mA=0;
    printf("\n\nEnter a current in MILLIAMPS (Enter `0' to terminate)...  "); GetInput();                   //Get input current in milliamps.
    if(!I){printf("\nProgram halted by user input.\n\n"); break;}
    I=(int)(I*10)/10.0;
    X=192; while((X-=S)>F){
      Y=192; while((Y-=S)>F){
        if(I>0){Z=1250/(1.0/(1.0/RT[X]+1.0/RT[Y])); if(Z<=I&&Z>mA){R1=X; R2=Y; mA=Z;}}                      //PARALLEL
        else {Z=1250/(RT[X]+RT[Y]); if(Z<=-I&&Z>mA){R1=X; R2=Y; mA=Z;}}                                     //SERIES
      }
    }
    if(R2<R1){N=R1; R1=R2; R2=N;}
    printf("\n  Wanted Current: %0.1f mA \t(Rounded DOWN to nearest 100 æA.)",I*(((I>0)<<1)-1));
    printf("\n  Actual Current: %0.1f mA \t(Rounded to nearest 100 æA.)",mA);
    sprintf(S1,"%f",RT[R1]); KillZero(S1); sprintf(S2,"%f",RT[R2]); KillZero(S2);
    printf("\n\n  Resistor Value:      %s\t%s\t  Ohms",S1,S2);
    if(I>0)printf("\n  Resistor Power:     %0.3f\t%0.3f\t  Watts",1.5625/RT[R1],1.5625/RT[R2]);             //PARALLEL
    else printf("\n  Resistor Power:     %0.3f\t%0.3f\t  Watts",mA*mA*1E-6*RT[R1],mA*mA*1E-6*RT[R2]);       //SERIES
    printf("\n\n  Network Config: %s",G[I<0]);
  }
  return 0;
}

void GetInput(){char X[8];
  while(fgets(STR,sizeof(STR)-1,stdin)){
    if(*STR!=32&&(sscanf(STR,"%lf%[^\n]",&I,X)==1)){fseek(stdin,0,SEEK_END); break;}
    printf("\rInvalid Entry!  Enter a number or `0' to exit...  ");
  }
  if(I<-Imax||I>Imax){printf("\rOut Of Range!  Enter a number between -5000 and 5000, (Use '0' to exit)...  "); GetInput();}
}

void KillZero(char *S){char *D=0,*p=S;
  while(*++p)if(*p=='.'||*p==',')D=p; if(D){while(*--p=='0')*p=0; if(p==D)*p=0;}
}
