#include <stdio.h>
#include <iostream>
#include <cmath>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>
#include <locale.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include "imgui_stdlib.h"

using namespace std;
using namespace std::chrono;
char* locale = setlocale(LC_ALL, "");

static void key_callback(GLFWwindow* window, int key, int scancode, int action,
                         int mods) {
   if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
      glfwSetWindowShouldClose(window, GL_TRUE);
}
struct KM {
   int number;   // How many Kilometrage points
   float value;  // value of the KM
};
struct BCanal { // structure for the branch or main or diversion canal.
   float length;              // Length of the branch or diversion canal
   int As;                    // Area served of the branch canal
   float PR[2] = {0.0, 0.0};  // partial regulator location on canal.
   int NDist;                 // Number of distributary canals
   float PRv[2] = {0.0, 0.0}; // Compensation ratio of the branch canal for example 0.3
   bool has_dI = false;  // Does the BCanal has direct Irrigation ?
   float DiAs;
} yCanal[1];

struct DCanal { // structure for distributary canal
   int As;  // Area served by the distributary Canal
   float PRv;
   float KMi;
   int AA = 0; // A down stream
   int BB = 0;  // B down stream
   float A = 0, B = 0, C = 0;
   int CC = 0;
   int AaCu = 0;
   int AaCd = 0;
   int CaBu = 0;
   int CaBd = 0;
   int AaBu;
   int BaAu;
   int AaBd;
   int BaAd;
   int mxAsu, mxAsd; // Area served design
   float Q, q;  // discharge values
   string location;  // the location of the suggested construction of
   bool upperstream = false;  // upperstream distributary
   bool downstream = false;   // downstream distributary
} iCanal[20];

float discharge(int As, float FWD, float k) {
   float WD = k * FWD;                   // Water Duty;
   const int convert_units = 86400;      // 24 * 60 * 60
   float Q = (As * WD) / convert_units;  // calculate the discharge
   return Q;
};
int maxof(int x, int y, int z) {  // function to find the maximum of three integer numbers
   int maxim = 0;
   if (x > y && x > z) {
      maxim = x;
   } else if (y > z) {
      maxim = y;
   } else {
      maxim = z;
   }
   return maxim;
}
struct canaldim{
float Qb,qb;
float Qd,qd ;
int n_1 = 44; // 1 / n
int slope = 10; // i cpk
float Z=1.0;
float bd=2.0;
}dim;
float longitudinal(float z,float bd,float Q,float N,float si){
    float Ax = pow((z + bd) ,1.6666666667);
    float Ay = pow( ( 2 * sqrt(pow(z,2) + 1) )+ bd , 0.6666666667);
    float Axy = Ax / Ay;
    float xZ = pow((si*pow(10,-5)),0.5 ) * N;
    float dZ = Q / xZ;
    float AB = dZ / Axy;
    dim.Qd = pow(AB , 0.375) ;
    return dim.Qd;
}
bool inRange(int low, int high, int x) { return ((x - high) * (x - low) <= 0); } // function to find if value within range to find the partial regulator locations
ofstream logres("log.txt");
float rotation3(int number, float c_ratio, int Asi, int rcycles, int FWD,
                float K) {
   struct DCanal xCanal[number];
   struct KM vKM[number];
   struct canaldim longValues[number];
   int Calc = 0;
   // float AA[number][2];
   // float PRv[2]={0.0,0.0};
   ofstream result("results_three_turn_rotation.csv");

   yCanal[0].As = Asi;

   result << "Calculation sheet for Irrigation rotation By ENG.Mohamed Gamal "
             "and Dr.Mohamed Anas\n\n";
   result << "KM,A(up),A(down),B(up),B(down),C(up),C(down),B(up) +cr "
             "A(up),B(down) +cr A(down),c(up) + cr B(up),C(down) + cr "
             "B(down),A(up) + cr C(up),A(down) + cr "
             "C(down),As(up),As(down),Q(up) m^3/sec,Q(down) m^3/sec, b(Q upstream),d(Q upstream),b(Q downstream),d(Q downstream)\n";

   for (int i = 0; i < number; i++) {
      xCanal[i].As = iCanal[i].As;
      xCanal[i].location = iCanal[i].location;
      // xCanal[i].location[6] = "\0";
      // xCanal[i].location = iCanal[i].location;
      // float FrV[number];
      // FrV[i] = iCanal[i].KMi;
      if (xCanal[i].As > 1) {
         Calc = Calc + xCanal[i].As;
      } else {
         Calc = Calc;
      }
      int AsU = Asi / 3;
      int MinR = AsU - 1001;
      int MaxR = AsU + 1001;
      vKM[i].number = i;

      if (iCanal[i].KMi == 0) {
         vKM[i].value = 0;
      } else {
         vKM[i].value = iCanal[i].KMi;
      }
      if (vKM[i].value == vKM[i - 1].value && vKM[i].value != 0) {
         logres << "*** Detected two distributary Canals on the same "
                "kilometrage.\n";
         logres << "--- Correcting values in memory..\n";
         xCanal[i].As += xCanal[i - 1].As;
         xCanal[i - 1].AA = 0;
         vKM[i].value = vKM[i - 1].value;
         vKM[i].number = vKM[i].number - 1;
         i = i - 1;
         printf(
             "New As for two distributaries on the same Kilometrage KM "
             "=%f\n",
             vKM[i].value);
      }
      if (inRange(MinR, MaxR, Calc)) {
         logres << "Partial regulator location is at downstream => KM: "
             << vKM[i].value << "\n";

         yCanal[0].PR[0] = vKM[i].value;
         yCanal[0].PRv[0] = Calc;
         logres << "1st Partial regulator location is at downstream => KM: "
             << vKM[i].value << "\n";

      } else if (inRange(MinR, MaxR, Calc - yCanal[0].PRv[0])) {
         yCanal[0].PRv[1] = Calc - yCanal[0].PRv[0];
         yCanal[0].PR[1] = vKM[i].value;
         logres << "2nd partial regulator is at downstream => KM: "
             << yCanal[0].PR[1] << "\n";
      } else {
         logres << "No Partial regulator.\n";
      }
      if (xCanal[i].location == "left") {
         // std::cout << "Dist Canal is left with As= " << xCanal[i].As << "
         // now total= " << Calc << std::endl;
         logres << "Dist Canal is left with As= " << xCanal[i].As
             << " now total= " << Calc << "\n";

      } else if (xCanal[i].location == "right") {
         logres << "Dist Canal is right with As= " << xCanal[i].As
             << " now total= " << Calc << "\n";
      } else {
         logres << "No distributary canal => " << vKM[i].value << std::endl;
      }
      if (i == number - 1) {
         int CAs = Asi - (Calc);
         xCanal[i].As = CAs;
         yCanal[0].has_dI = true;
         yCanal[0].DiAs = CAs;
         logres << "Total calculated area served from distributaries = " << Calc
             << "\n";
         logres << "Direct Irrigation Area = " << CAs << "\n";
      }
   }
   for (int j = 0; j < number; j++) {
      if (vKM[j].value == 0) {
         xCanal[j].A = yCanal[0].PRv[0];
         xCanal[j].AA = yCanal[0].PRv[0] - xCanal[j].As;
         xCanal[j].B = yCanal[0].PRv[1];
         xCanal[j].BB = xCanal[j].B;
         xCanal[j].C = yCanal[0].As - yCanal[0].PRv[0] - yCanal[0].PRv[1];
         xCanal[j].CC = xCanal[j].C;
         xCanal[j].BaAu = xCanal[j].B + c_ratio * xCanal[j].A;
         xCanal[j].BaAd = xCanal[j].BB + c_ratio * xCanal[j].AA;
         xCanal[j].CaBu = xCanal[j].C + c_ratio * xCanal[j].B;
         xCanal[j].CaBd = xCanal[j].CC + c_ratio * xCanal[j].BB;
         xCanal[j].AaCu = xCanal[j].A + c_ratio * xCanal[j].C;
         xCanal[j].AaCd = xCanal[j].AA + c_ratio * xCanal[j].CC;
         xCanal[j].mxAsu =
             maxof(xCanal[j].AaCu, xCanal[j].BaAu, xCanal[j].CaBu);
         xCanal[j].mxAsd =
             maxof(xCanal[j].AaCd, xCanal[j].BaAd, xCanal[j].CaBd);
         xCanal[j].Q = discharge(xCanal[j].mxAsu, FWD, K);
         longValues[j].Qd = longitudinal(dim.Z,dim.bd,xCanal[j].Q,dim.n_1,dim.slope);
         longValues[j].Qb = dim.bd * longValues[j].Qd;
         xCanal[j].q = discharge(xCanal[j].mxAsd, FWD, K);
         longValues[j].qd = longitudinal(dim.Z,dim.bd,xCanal[j].q,dim.n_1,dim.slope);
         longValues[j].qb = dim.bd * longValues[j].qd;
         xCanal[j].downstream = true;
         xCanal[j].upperstream = true;
      } else if (vKM[j].value != 0 && vKM[j].value <= yCanal[0].PR[0] &&
                 xCanal[j].upperstream == false) {
         xCanal[j].A = xCanal[j - 1].AA;
         xCanal[j].AA = xCanal[j].A - xCanal[j].As;
         xCanal[j].B = xCanal[j - 1].BB;
         xCanal[j].BB = xCanal[j].B;
         xCanal[j].C = xCanal[j - 1].CC;
         xCanal[j].CC = xCanal[j].C;
         xCanal[j].BaAu = xCanal[j].B + c_ratio * xCanal[j].A;
         xCanal[j].BaAd = xCanal[j].BB + c_ratio * xCanal[j].AA;
         xCanal[j].CaBu = xCanal[j].C + c_ratio * xCanal[j].B;
         xCanal[j].CaBd = xCanal[j].CC + c_ratio * xCanal[j].BB;
         xCanal[j].AaCu = xCanal[j].A + c_ratio * xCanal[j].C;
         xCanal[j].AaCd = xCanal[j].AA + c_ratio * xCanal[j].CC;
         xCanal[j].mxAsu =
             maxof(xCanal[j].AaCu, xCanal[j].BaAu, xCanal[j].CaBu);
         xCanal[j].mxAsd =
             maxof(xCanal[j].AaCd, xCanal[j].BaAd, xCanal[j].CaBd);
         xCanal[j].Q = discharge(xCanal[j].mxAsu, FWD, K);
         longValues[j].Qd = longitudinal(dim.Z,dim.bd,xCanal[j].Q,dim.n_1,dim.slope);
         longValues[j].Qb = dim.bd * longValues[j].Qd;
         xCanal[j].q = discharge(xCanal[j].mxAsd, FWD, K);
         longValues[j].qd = longitudinal(dim.Z,dim.bd,xCanal[j].q,dim.n_1,dim.slope);
         longValues[j].qb = dim.bd * longValues[j].qd;
         xCanal[j].upperstream = true;
         xCanal[j].downstream = false;

      } else if (vKM[j].value != 0 && vKM[j].value <= yCanal[0].PR[1] &&
                 xCanal[j].upperstream == false) {
         xCanal[j].A = xCanal[j - 1].AA;
         xCanal[j].AA = xCanal[j].A;
         xCanal[j].B = xCanal[j - 1].BB;
         xCanal[j].BB = xCanal[j].B - xCanal[j].As;
         xCanal[j].C = xCanal[j - 1].CC;
         xCanal[j].CC = xCanal[j].C;
         xCanal[j].BaAu = xCanal[j].B + c_ratio * xCanal[j].A;
         xCanal[j].BaAd = xCanal[j].BB + c_ratio * xCanal[j].AA;
         xCanal[j].CaBu = xCanal[j].C + c_ratio * xCanal[j].B;
         xCanal[j].CaBd = xCanal[j].CC + c_ratio * xCanal[j].BB;
         xCanal[j].AaCu = xCanal[j].A + c_ratio * xCanal[j].C;
         xCanal[j].AaCd = xCanal[j].AA + c_ratio * xCanal[j].CC;
         xCanal[j].mxAsu =
             maxof(xCanal[j].AaCu, xCanal[j].BaAu, xCanal[j].CaBu);
         xCanal[j].mxAsd =
             maxof(xCanal[j].AaCd, xCanal[j].BaAd, xCanal[j].CaBd);
         xCanal[j].Q = discharge(xCanal[j].mxAsu, FWD, K);
         longValues[j].Qd = longitudinal(dim.Z,dim.bd,xCanal[j].Q,dim.n_1,dim.slope);
         longValues[j].Qb = dim.bd * longValues[j].Qd;
         xCanal[j].q = discharge(xCanal[j].mxAsd, FWD, K);
         longValues[j].qd = longitudinal(dim.Z,dim.bd,xCanal[j].q,dim.n_1,dim.slope);
         longValues[j].qb = dim.bd * longValues[j].qd;
         xCanal[j].downstream = true;
         xCanal[j].upperstream = false;

      } else {
         xCanal[j].A = 0;
         xCanal[j].AA = 0;
         // xCanal[j].B = 0;
         // xCanal[j].BB=0;
      }

      if (vKM[j].value > yCanal[0].PR[1] && xCanal[j].upperstream == false &&
          j != number - 1) {
         xCanal[j].C = xCanal[j - 1].CC;
         xCanal[j].CC = xCanal[j].C - xCanal[j].As;
         xCanal[j].BaAu = xCanal[j].B + c_ratio * xCanal[j].A;
         xCanal[j].BaAd = xCanal[j].BB + c_ratio * xCanal[j].AA;
         xCanal[j].CaBu = xCanal[j].C + c_ratio * xCanal[j].B;
         xCanal[j].CaBd = xCanal[j].CC + c_ratio * xCanal[j].BB;
         xCanal[j].AaCu = xCanal[j].A + c_ratio * xCanal[j].C;
         xCanal[j].AaCd = xCanal[j].AA + c_ratio * xCanal[j].CC;
         xCanal[j].mxAsu =
             maxof(xCanal[j].AaCu, xCanal[j].BaAu, xCanal[j].CaBu);
         xCanal[j].mxAsd =
             maxof(xCanal[j].AaCd, xCanal[j].BaAd, xCanal[j].CaBd);
         xCanal[j].Q = discharge(xCanal[j].mxAsu, FWD, K);
         longValues[j].Qd = longitudinal(dim.Z,dim.bd,xCanal[j].Q,dim.n_1,dim.slope);
         longValues[j].Qb = dim.bd * longValues[j].Qd;
         xCanal[j].q = discharge(xCanal[j].mxAsd, FWD, K);
         xCanal[j].downstream = true;
         xCanal[j].upperstream = true;
      } else if (j == number - 1) {
         xCanal[j].C = xCanal[j - 1].CC;
         xCanal[j].CC = xCanal[j].C / 2;
         xCanal[j].BaAu = xCanal[j].B + c_ratio * xCanal[j].A;
         xCanal[j].BaAd = xCanal[j].BB + c_ratio * xCanal[j].AA;
         xCanal[j].CaBu = xCanal[j].C + c_ratio * xCanal[j].B;
         xCanal[j].CaBd = xCanal[j].CC + c_ratio * xCanal[j].BB;
         xCanal[j].AaCu = xCanal[j].A + c_ratio * xCanal[j].C;
         xCanal[j].AaCd = xCanal[j].AA + c_ratio * xCanal[j].CC;
         xCanal[j].mxAsu =
             maxof(xCanal[j].AaCu, xCanal[j].BaAu, xCanal[j].CaBu);
         xCanal[j].mxAsd =
             maxof(xCanal[j].AaCd, xCanal[j].BaAd, xCanal[j].CaBd);
         xCanal[j].Q = discharge(xCanal[j].mxAsu, FWD, K);
         longValues[j].Qd = longitudinal(dim.Z,dim.bd,xCanal[j].Q,dim.n_1,dim.slope);
         longValues[j].Qb = dim.bd * longValues[j].Qd;
         xCanal[j].q = discharge(xCanal[j].mxAsd, FWD, K);
         longValues[j].qd = longitudinal(dim.Z,dim.bd,xCanal[j].q,dim.n_1,dim.slope);
         longValues[j].qb = dim.bd * longValues[j].qd;
         xCanal[j].downstream = true;
         xCanal[j].upperstream = true;
      }

      result << vKM[j].value << "," << xCanal[j].A << "," << xCanal[j].AA << ","
             << xCanal[j].B << "," << xCanal[j].BB << "," << xCanal[j].C << ","
             << xCanal[j].CC << "," << xCanal[j].BaAu << "," << xCanal[j].BaAd
             << "," << xCanal[j].CaBu << "," << xCanal[j].CaBd << ","
             << xCanal[j].AaCu << "," << xCanal[j].AaCd << ","
             << xCanal[j].mxAsu << "," << xCanal[j].mxAsd << "," << xCanal[j].Q
             << "," << xCanal[j].q << "," << longValues[j].Qb << ","
             <<longValues[j].Qd<< "," <<longValues[j].qb <<","<< longValues[j].qd <<"\n";
   }

   result.close();

   logres.close();

   return 0;
};
double rotation(int number, float c_ratio, int Asi, int rcycles, int FWD,
                float K) {
   struct DCanal xCanal[number];
   struct KM vKM[number];
   struct canaldim longValues[number];
   int Calc = 0;
   // float AA[number][2];
   // float PRv[2]={0.0,0.0};
   ofstream result("results_two_turn_rotation.csv");

   yCanal[0].As = Asi;

   result << "\n";
   result << "KM,A(up),A(down),B(up),B(down),A(up) +cr B(up),A(down) +cr "
             "B(down),B(up) + cr A(up),B(down) + cr "
             "A(down),As(up),As(down),Q(up) m^3/sec,Q(down) m^3/sec,b (upstream)  ,d (upstream),b(downstream),d(downstream)\n";
   for (int i = 0; i < number; i++) {
      xCanal[i].As = iCanal[i].As;
      xCanal[i].location = iCanal[i].location;
      //        xCanal[i].location[6] = "\0";
      //      float FrV[number];
      //     FrV[i] = iCanal[i].KMi;
      if (xCanal[i].As > 1) {
         Calc = Calc + xCanal[i].As;
      } else {
         Calc = Calc;
      }
      int AsU = Asi / 2;
      int MinR = AsU - 1001;
      int MaxR = AsU + 1001;
      vKM[i].number = i;

      if (iCanal[i].KMi == 0) {
         vKM[i].value = 0;
      } else {
         vKM[i].value = iCanal[i].KMi;
      }
      if (vKM[i].value == vKM[i - 1].value && vKM[i].value != 0) {
         logres << "*** Detected two distributary Canals on the same "
                "kilometrage.\n";
         logres << "--- Correcting values in memory..\n";
         xCanal[i].As += xCanal[i - 1].As;
         xCanal[i - 1].AA = 0;
         vKM[i].value = vKM[i - 1].value;
         vKM[i].number = vKM[i].number - 1;
         i = i - 1;
      }
      if (inRange(MinR, MaxR, Calc)) {
         logres << "Partial regulator location is at downstream => KM: "
             << vKM[i].value << "\n";
         yCanal[0].PR[0] = vKM[i].value;
         yCanal[0].PRv[0] = Calc;
      } else {
         logres << "No partial regulator at this location.\n";
      }
      if (xCanal[i].location == "left") {
         // std::cout << "Dist Canal is left with As= " << xCanal[i].As << "
         // now total= " << Calc << std::endl;
         logres << "Dist Canal is left with As= " << xCanal[i].As
             << " now total= " << Calc << "\n";

      } else if (xCanal[i].location == "right") {
         // std::cout << "Dist Canal is right with As= " << xCanal[i].As << "
         // now total= " << Calc << std::endl;
         logres << "Dist Canal is right with As= " << xCanal[i].As
             << " now total= " << Calc << "\n";
      } else {
         // std::cout << xCanal[i].location;
         // std::cout << "No distributary canal => " << vKM[i].value <<
         // std::endl;
         logres << "No distributary canal => " << vKM[i].value << std::endl;
         // inRange(MinR, MaxR, Calc)? cout << "Partial regulator location is
         // betwean section[ " << i+1 << " ] down stream and section [ " <<
         // i+2 << " ] upstream.\n":  cout  <<"No partial regulator at this
         // location.\n";
      }
      if (i == number - 1) {
         int CAs = Asi - (Calc);
         xCanal[i].As = CAs;
         yCanal[0].has_dI = true;
         yCanal[0].DiAs = CAs;
         // std::cout << "Total calculated area served from distributaries =
         // " << Calc << std::endl;
         logres << "Total calculated area served from distributaries = " << Calc
             << "\n";
         // std::cout << "Direct Irrigation Area = " << CAs << std::endl;
         logres << "Direct Irrigation Area = " << CAs << "\n";
      }
   }
   for (int j = 0; j < number; j++) {
      if (vKM[j].value == 0) {
         xCanal[j].A = yCanal[0].PRv[0];
         xCanal[j].AA = yCanal[0].PRv[0] - xCanal[j].As;
         xCanal[j].B = yCanal[0].As - yCanal[0].PRv[0];
         xCanal[j].BB = xCanal[j].B;
         xCanal[j].AaBu = xCanal[j].A + c_ratio * xCanal[j].B;
         xCanal[j].AaBd = xCanal[j].AA + c_ratio * xCanal[j].BB;
         xCanal[j].BaAu = xCanal[j].B + c_ratio * xCanal[j].A;
         xCanal[j].BaAd = xCanal[j].BB + c_ratio * xCanal[j].AA;
         xCanal[j].mxAsu = max(xCanal[j].AaBu, xCanal[j].BaAu);
         xCanal[j].mxAsd = max(xCanal[j].BaAd, xCanal[j].AaBd);
         xCanal[j].Q = discharge(xCanal[j].mxAsu, FWD, K);
         xCanal[j].q = discharge(xCanal[j].mxAsd, FWD, K);
         longValues[j].Qd = longitudinal(dim.Z,dim.bd,xCanal[j].Q,dim.n_1,dim.slope);
         longValues[j].Qb = dim.bd * longValues[j].Qd;
         longValues[j].qd = longitudinal(dim.Z,dim.bd,xCanal[j].q,dim.n_1,dim.slope);
         longValues[j].qb = dim.bd * longValues[j].qd;
         xCanal[j].downstream = true;
         xCanal[j].upperstream = true;
      } else if (vKM[j].value != 0 && vKM[j].value <= yCanal[0].PR[0] &&
                 xCanal[j].upperstream == false) {
         xCanal[j].A = xCanal[j - 1].AA;
         xCanal[j].AA = xCanal[j].A - xCanal[j].As;
         xCanal[j].B = yCanal[0].As - yCanal[0].PRv[0];
         xCanal[j].BB = xCanal[j].B;
         xCanal[j].AaBu = xCanal[j].A + c_ratio * xCanal[j].B;
         xCanal[j].AaBd = xCanal[j].AA + c_ratio * xCanal[j].BB;
         xCanal[j].BaAu = xCanal[j].B + c_ratio * xCanal[j].A;
         xCanal[j].BaAd = xCanal[j].BB + c_ratio * xCanal[j].AA;
         xCanal[j].mxAsu = max(xCanal[j].AaBu, xCanal[j].BaAu);
         xCanal[j].mxAsd = max(xCanal[j].BaAd, xCanal[j].AaBd);
         xCanal[j].Q = discharge(xCanal[j].mxAsu, FWD, K);
         xCanal[j].q = discharge(xCanal[j].mxAsd, FWD, K);
         longValues[j].Qd = longitudinal(dim.Z,dim.bd,xCanal[j].Q,dim.n_1,dim.slope);
         longValues[j].Qb = dim.bd * longValues[j].Qd;
         longValues[j].qd = longitudinal(dim.Z,dim.bd,xCanal[j].q,dim.n_1,dim.slope);
         longValues[j].qb = dim.bd * longValues[j].qd;
         xCanal[j].upperstream = true;
         xCanal[j].downstream = false;
      } else if (vKM[j].value != 0 && vKM[j].value <= yCanal[0].PR[0] &&
                 xCanal[j].upperstream == true &&
                 xCanal[j].downstream == false) {
         xCanal[j].A = xCanal[j - 1].AA;
         xCanal[j].AA = xCanal[j].A - xCanal[j].As;
         xCanal[j].B = yCanal[0].As - yCanal[0].PRv[0];
         xCanal[j].BB = xCanal[j].B;
         xCanal[j].AaBu = xCanal[j].A + c_ratio * xCanal[j].B;
         xCanal[j].AaBd = xCanal[j].AA + c_ratio * xCanal[j].BB;
         xCanal[j].BaAu = xCanal[j].B + c_ratio * xCanal[j].A;
         xCanal[j].BaAd = xCanal[j].BB + c_ratio * xCanal[j].AA;
         xCanal[j].mxAsu = max(xCanal[j].AaBu, xCanal[j].BaAu);
         xCanal[j].mxAsd = max(xCanal[j].BaAd, xCanal[j].AaBd);
         xCanal[j].Q = discharge(xCanal[j].mxAsu, FWD, K);
         xCanal[j].q = discharge(xCanal[j].mxAsd, FWD, K);
         longValues[j].Qd = longitudinal(dim.Z,dim.bd,xCanal[j].Q,dim.n_1,dim.slope);
         longValues[j].Qb = dim.bd * longValues[j].Qd;
         longValues[j].qd = longitudinal(dim.Z,dim.bd,xCanal[j].q,dim.n_1,dim.slope);
         longValues[j].qb = dim.bd * longValues[j].qd;
         xCanal[j].downstream = true;
         xCanal[j].upperstream = false;
      } else if (vKM[j].value > yCanal[0].PR[0] &&
                 xCanal[j].upperstream == false && j != number - 1) {
         xCanal[j].B = xCanal[j - 1].BB;
         xCanal[j].BB = xCanal[j].B - xCanal[j].As;
         xCanal[j].AaBu = xCanal[j].A + c_ratio * xCanal[j].B;
         xCanal[j].AaBd = xCanal[j].AA + c_ratio * xCanal[j].BB;
         xCanal[j].BaAu = xCanal[j].B + c_ratio * xCanal[j].A;
         xCanal[j].BaAd = xCanal[j].BB + c_ratio * xCanal[j].AA;
         xCanal[j].mxAsu = max(xCanal[j].AaBu, xCanal[j].BaAu);
         xCanal[j].mxAsd = max(xCanal[j].BaAd, xCanal[j].AaBd);
         xCanal[j].Q = discharge(xCanal[j].mxAsu, FWD, K);
         xCanal[j].q = discharge(xCanal[j].mxAsd, FWD, K);
         longValues[j].Qd = longitudinal(dim.Z,dim.bd,xCanal[j].Q,dim.n_1,dim.slope);
         longValues[j].Qb = dim.bd * longValues[j].Qd;
         longValues[j].qd = longitudinal(dim.Z,dim.bd,xCanal[j].q,dim.n_1,dim.slope);
         longValues[j].qb = dim.bd * longValues[j].qd;
         xCanal[j].downstream = true;
         xCanal[j].upperstream = true;
      } else if (j == number - 1) {
         xCanal[j].B = xCanal[j - 1].BB;
         xCanal[j].BB = xCanal[j - 1].BB / 2;
         xCanal[j].AaBu = xCanal[j].A + c_ratio * xCanal[j].B;
         xCanal[j].AaBd = xCanal[j].AA + c_ratio * xCanal[j].BB;
         xCanal[j].BaAu = xCanal[j].B + c_ratio * xCanal[j].A;
         xCanal[j].BaAd = xCanal[j].BB + c_ratio * xCanal[j].AA;
         xCanal[j].mxAsu = max(xCanal[j].AaBu, xCanal[j].BaAu);
         xCanal[j].mxAsd = max(xCanal[j].BaAd, xCanal[j].AaBd);
         xCanal[j].Q = discharge(xCanal[j].mxAsu, FWD, K);
         xCanal[j].q = discharge(xCanal[j].mxAsd, FWD, K);
         longValues[j].Qd = longitudinal(dim.Z,dim.bd,xCanal[j].Q,dim.n_1,dim.slope);
         longValues[j].Qb = dim.bd * longValues[j].Qd;
         longValues[j].qd = longitudinal(dim.Z,dim.bd,xCanal[j].q,dim.n_1,dim.slope);
         longValues[j].qb = dim.bd * longValues[j].qd;
      } else {
         xCanal[j].A = 0;
         xCanal[j].AA = 0;
      }

      result << vKM[j].value << "," << xCanal[j].A << "," << xCanal[j].AA << ","
             << xCanal[j].B << "," << xCanal[j].BB << "," << xCanal[j].AaBu
             << "," << xCanal[j].AaBd << "," << xCanal[j].BaAu << ","
             << xCanal[j].BaAd << "," << xCanal[j].mxAsu << ","
             << xCanal[j].mxAsd << "," << xCanal[j].Q << "," << xCanal[j].q
             << ", " << longValues[j].Qb << "," << longValues[j].Qd << " ,"
             << longValues[j].qb << "," << longValues[j].qd << " ,"
             << "\n";
   }

   logres.close();
   result.close();
   return 0;
};

static void glfw_error_callback(int error, const char* description) {
   fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**) {
   // Setup window
   glfwSetErrorCallback(glfw_error_callback);
   if (!glfwInit()) return 1;

      // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
   // GL ES 2.0 + GLSL 100
   const char* glsl_version = "#version 100";
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
   glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
   // GL 3.2 + GLSL 150
   const char* glsl_version = "#version 150";
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
   // GL 3.0 + GLSL 130
   const char* glsl_version = "#version 130";
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
   // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
   // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

   // Create window with graphics context
   GLFWwindow* window =
       glfwCreateWindow(1380, 720, "Irrigation Works v1.0b", NULL, NULL);
   if (window == NULL) return 1;
   glfwMakeContextCurrent(window);
   glfwSetKeyCallback(window, key_callback);
   glfwSwapInterval(1);  // Enable vsync

   // Setup Dear ImGui context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO();
   (void)io;
   io.ConfigFlags |=
       ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
   // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable
   // Gamepad Controls

   // Setup Dear ImGui style
   // ImGui::StyleColorsDark();
   ImGui::StyleColorsClassic();

   // Setup Platform/Renderer backends
   ImGui_ImplGlfw_InitForOpenGL(window, true);
   ImGui_ImplOpenGL3_Init(glsl_version);
   // Our state
   ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

   bool show__canal_window = false;
   bool clay = true;
   bool silt = false;
   bool sand = false;

   int FWD = 50, Kmn = 8, As = 23000;
   int rotation_cycles = 2;
   string As_loc;
   float k = 1.2, cRatio = 0.3;
   bool calculation_running = false;
   bool drawing = false;
   bool about = false;
   int elapsed = 0;
   bool soil_state = false;
   // Main loop
   while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();

      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      {
         ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
         ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
         if (ImGui::Begin("Main")) {
            ImGui::InputInt("Number of sections", &Kmn);
            ImGui::InputInt("Area served by main or branch/diversion canal",
                            &As);
            ImGui::InputInt("Number of rotation cycles", &rotation_cycles);
            ImGui::InputInt("Field water duty (m^3/fed/day)", &FWD);
            ImGui::SliderFloat(
                "Factor to calculate WD (1.20) for branch canal.", &k, 1.0f,
                1.5f);
            ImGui::SliderFloat(
                "Compensation ratio (0.3)", &cRatio, 0.0f,
                1.0f);  // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::Text("Given b = ");
            ImGui::SameLine();
            ImGui::InputFloat("b = ",&dim.bd,0.0f,100.0f);
            ImGui::Text("Soil");
            ImGui::SameLine();
            ImGui::Checkbox("Clay",&clay);
            ImGui::SameLine();
            ImGui::Checkbox("Silt",&silt);
            ImGui::SameLine();
            ImGui::Checkbox("Sand",&sand);
            ImGui::Checkbox("Kilometrage window", &show__canal_window);
            ImGui::ColorEdit3("Change BackGround",
                              (float*)&clear_color);  // Edit 3 floats representing a color
            if(soil_state){

            }else if(clay){
                dim.Z=1.0;
                silt = false;
                sand = false;
            }else if(silt){
                dim.Z=1.5;
                clay = false;
                sand = false;
            }else if(sand){
                dim.Z=2.0;
                silt = false;
                clay = false;
            }
            if (about) {
               ImGui::OpenPopup("About Irrigation Works.");

               if (ImGui::BeginPopupModal("About Irrigation Works.", &about)) {
                  ImGui::Text("Irrigation Works v1.0b\n");
                  ImGui::Text("Programming Language: ");
                  ImGui::SameLine();
                  ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.f), "C/C++\n");
                  ImGui::Text("Libraries: ");
                  ImGui::SameLine();
                  ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.0f, 1.f),
                                     "OpenGL, GLFW, and ImGui\n");
                  ImGui::Text("Software Engineer: ");
                  ImGui::SameLine();
                  ImGui::TextColored(ImVec4(0.8, 0.0f, 0.0f, 1.f),
                                     "Eng.Mohamed Jamal github.com/mohamed-ghayyad");
                  ImGui::Text("Supervisor: ");
                  ImGui::SameLine();
                  ImGui::TextColored(ImVec4(0.8, 0.8f, 0.0f, 1.f),
                                     "Dr.Mohamed Anas");
                  ImGui::Text("Software testing and Linux Guru: ");
                  ImGui::SameLine();
                  ImGui::TextColored(ImVec4(0.8, 0.0f, 0.0f, 1.f), "James Ross github.com/itzexor");
                  if (ImGui::Button("Close")) {
                     ImGui::CloseCurrentPopup();
                     about = false;
                  }

                  ImGui::EndPopup();
               }
            } else if (ImGui::Button("About")) {
               about = true;
            }
            if (Kmn > 20) {
               ImGui::OpenPopup("Limit error number of sections");
               bool opdn = true;
               if (ImGui::BeginPopupModal("Limit error number of sections",
                                          &opdn,
                                          ImGuiWindowFlags_NoDecoration)) {
                  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.0f, 1.0f),
                                     "Only 20 sections is allowed to be "
                                     "created dynamically.");
                  if (ImGui::Button("Close")) {
                     ImGui::CloseCurrentPopup();
                     Kmn = 20;
                  }

                  ImGui::EndPopup();
               }
            }
            if (FWD > 100) {
               ImGui::OpenPopup("Error field water duty");
               bool opdn = true;
               if (ImGui::BeginPopupModal("Error field water duty", &opdn,
                                          ImGuiWindowFlags_NoDecoration)) {
                  ImGui::TextColored(
                      ImVec4(0.7f, 0.7f, 0.0f, 1.0f),
                      "Field water duty can't be larger than 100");
                  if (ImGui::Button("Close")) {
                     ImGui::CloseCurrentPopup();
                     FWD = 50;
                  }

                  ImGui::EndPopup();
               }
            }
            if (As > 300000) {
               ImGui::OpenPopup("Limit error area served");
               bool opdn2 = true;
               if (ImGui::BeginPopupModal("Limit error area served", &opdn2,
                                          ImGuiWindowFlags_NoDecoration)) {
                  ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.0f, 1.0f),
                                     "Area served can't be larger than 300000");
                  if (ImGui::Button("Close")) {
                     ImGui::CloseCurrentPopup();
                     As = 23000;
                  }

                  ImGui::EndPopup();
               }
            }
            if (show__canal_window) {
               if ((rotation_cycles == 2) || (rotation_cycles == 3)) {
               } else {
                  bool open = true;
                  ImGui::OpenPopup("Error");

                  if (ImGui::BeginPopupModal("Error", &open,
                                             ImGuiWindowFlags_NoDecoration)) {
                     ImGui::TextColored(
                         ImVec4(0.7f, 0.7f, 0.0f, 1.0f),
                         "Only two turn rotation or three turn rotation cycles "
                         "is supported.");
                     ImGui::TextColored(
                         ImVec4(0.5f, 0.5f, 0.0f, 1.0f),
                         "Click on the type of the desired rotation "
                         "cycles you need to change.");
                     if (ImGui::Button("Two turn rotation.")) {
                        ImGui::CloseCurrentPopup();
                        rotation_cycles = 2;
                     }
                     ImGui::SameLine(0.1f, 400.0f);
                     if (ImGui::Button("Three turn rotation.")) {
                        ImGui::CloseCurrentPopup();
                        rotation_cycles = 3;
                     }

                     ImGui::EndPopup();
                  }
               }

               ImGui::Begin("Kilometrage data", &show__canal_window);

               for (int m = 0; m < Kmn; m++) {
                  std::string locationstd = iCanal[m].location;
                  if (locationstd.length() > 6) {
                     ImGui::OpenPopup("Area served error");
                     bool opd32n = true;
                     if (ImGui::BeginPopupModal(
                             "Area served error", &opd32n,
                             ImGuiWindowFlags_NoDecoration)) {
                        ImGui::TextColored(
                            ImVec4(0.7f, 0.7f, 0.0f, 1.0f),
                            "Area served limit Error for security " // preventing possible buffer overflow.
                            "reasons.");
                        if (ImGui::Button("Close")) {
                           ImGui::CloseCurrentPopup();
                           iCanal[m].location = "left";
                        }
                        // iCanal[m].KMi =iCanal[m-1].KMi+1;
                        ImGui::EndPopup();
                     }
                  }
                  if (iCanal[m].KMi > 100) {
                     ImGui::OpenPopup("kilomitrage error");
                     bool opd32n = true;
                     if (ImGui::BeginPopupModal(
                             "kilomitrage error", &opd32n,
                             ImGuiWindowFlags_NoDecoration)) {
                        ImGui::TextColored(
                            ImVec4(0.7f, 0.7f, 0.0f, 1.0f),
                            "kilomitrage limit Error for security "
                            "reasons.");
                        if (ImGui::Button("Close")) {
                           ImGui::CloseCurrentPopup();
                           iCanal[m].KMi = iCanal[m - 1].KMi + 1;
                        }
                        // iCanal[m].KMi =iCanal[m-1].KMi+1;
                        ImGui::EndPopup();
                     }
                  }
                  if (iCanal[m].As > As) {
                     ImGui::OpenPopup("area served error");
                     bool opd32n = true;
                     if (ImGui::BeginPopupModal(
                             "area served error", &opd32n,
                             ImGuiWindowFlags_NoDecoration)) {
                        ImGui::TextColored(
                            ImVec4(0.8f, 0.8f, 0.8f, 1.0f),
                            "Area served of ditributary canal cannot "
                            "be larger than main canal.");
                        if (ImGui::Button("Close")) {
                           ImGui::CloseCurrentPopup();
                           iCanal[m].As = 0;
                        }
                        // iCanal[m].KMi =iCanal[m-1].KMi+1;
                        ImGui::EndPopup();
                     }
                  }
                  if(iCanal[m].KMi == iCanal[m-1].KMi && iCanal[m].KMi!=0){
                     ImGui::OpenPopup("kilometrage error");
                     bool opd32n = true;
                     if (ImGui::BeginPopupModal(
                             "kilometrage error", &opd32n)) {
                        ImGui::TextColored(
                            ImVec4(0.8f, 0.8f, 0.0f, 1.0f),
                            "Kilometrage value cannot be the same "
                            "as the previous value.");
                        if (ImGui::Button("Close")) {
                           ImGui::CloseCurrentPopup();
                           iCanal[m].KMi = iCanal[m-1].KMi +1;
                        }
                        // iCanal[m].KMi =iCanal[m-1].KMi+1;
                        ImGui::EndPopup();
                     }
                  }
                  ImGui::PushID(m);
                  ImGui::Text("Enter data for section[%i]", m);
                  ImGui::InputInt("Area served for distributary canal",
                                  &iCanal[m].As);

                  ImGui::InputText("Location of the canal",
                                   &iCanal[m].location);

                  ImGui::InputFloat("KM Location on the main/branch canal (KM)",
                                    &iCanal[m].KMi);

                  ImGui::Text("Area served for dist canal %i, KM [%f]",
                              iCanal[m].As, iCanal[m].KMi);
                  ImGui::PopID();
               }
               if (calculation_running) {
                  ImGui::OpenPopup("Calculation of the irrigation");
                  bool opdn4 = true;
                  if (ImGui::BeginPopupModal("Calculation of the irrigation",
                                             &opdn4,
                                             ImGuiWindowFlags_NoDecoration)) {
                     ImGui::TextColored(
                         ImVec4(0.8f, 0.8f, 0.0f, 1.0f),
                         "Calculated in %i milliseconds, results is saved and "
                         "click draw to show the layout.",
                         elapsed);
                     if (ImGui::Button("Close")) {
                        ImGui::CloseCurrentPopup();
                        calculation_running = false;
                     }
                     ImGui::SameLine(0.1f, 400.0f);
                     if (drawing) {
                        ImDrawList* dlines = ImGui::GetForegroundDrawList();
                        ImVec2 offset(400, 200);
                        dlines->AddRectFilled(
                            offset, ImVec2(offset.x + 500, offset.y + 200),
                            0xFFFFFFFF);
                        dlines->AddLine(ImVec2(offset.x + 10, offset.y + 100),
                                        ImVec2(offset.x + 490, offset.y + 100),
                                        0xFF0000FF);
                        dlines->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                                        ImVec2(offset.x + 150, offset.y + 10),
                                        ImColor(0, 0, 0, 255),
                                        "Irrigation rotation layout", 0, 0.0f,
                                        0);
                        for (int i = 0; i < Kmn; i++) {
                           const std::string AxisValue =
                               std::to_string((int)iCanal[i].KMi);
                           const std::string DistAS =
                               std::to_string((int)iCanal[i].As);
                           const std::string DiAs =
                               "DI = " + std::to_string((int)yCanal[0].DiAs) +
                               " Fed";
                           if (iCanal[i].location == "right") {
                              if (iCanal[i].KMi == 0) {
                                 dlines->AddLine(
                                     ImVec2(offset.x + 10, offset.y + 100),
                                     ImVec2(offset.x + 10, offset.y + 140),
                                     0xFF0000FF);
                                 dlines->AddText(
                                     ImGui::GetFont(), ImGui::GetFontSize(),
                                     ImVec2(offset.x + 10, offset.y + 140),
                                     ImColor(0, 0, 0, 255), DistAS.c_str(), 0,
                                     0.0f, 0);
                              } else {
                                 dlines->AddLine(
                                     ImVec2(offset.x + (iCanal[i].KMi * 20),
                                            offset.y + 100),
                                     ImVec2(offset.x + (iCanal[i].KMi * 20),
                                            offset.y + 140),
                                     0xFF0000FF);
                                 dlines->AddText(
                                     ImGui::GetFont(), ImGui::GetFontSize(),
                                     ImVec2(offset.x + (iCanal[i].KMi * 20),
                                            offset.y + 140),
                                     ImColor(0, 0, 0, 255), DistAS.c_str(), 0,
                                     0.0f, 0);
                              }

                           } else if (iCanal[i].location == "left") {
                              if (iCanal[i].KMi == 0) {
                                 dlines->AddLine(
                                     ImVec2(offset.x + 10, offset.y + 100),
                                     ImVec2(offset.x + 10, offset.y + 40),
                                     0xFF0000FF);
                                 dlines->AddText(
                                     ImGui::GetFont(), ImGui::GetFontSize(),
                                     ImVec2(offset.x + 10, offset.y + 40),
                                     ImColor(0, 0, 0, 255), DistAS.c_str(), 0,
                                     0.0f, 0);

                              } else {
                                 dlines->AddLine(
                                     ImVec2(offset.x + (iCanal[i].KMi * 20),
                                            offset.y + 100),
                                     ImVec2(offset.x + (iCanal[i].KMi * 20),
                                            offset.y + 40),
                                     0xFF0000FF);
                                 dlines->AddText(
                                     ImGui::GetFont(), ImGui::GetFontSize(),
                                     ImVec2(offset.x + (iCanal[i].KMi * 20),
                                            offset.y + 40),
                                     ImColor(0, 0, 0, 255), DistAS.c_str(), 0,
                                     0.0f, 0);
                              }
                           }
                           if (rotation_cycles == 2) {
                              dlines->AddLine(
                                  ImVec2(offset.x + (yCanal[0].PR[0] * 20 + 5),
                                         offset.y + 100),
                                  ImVec2(offset.x + (yCanal[0].PR[0] * 20 + 5),
                                         offset.y + 130),
                                  ImColor(0, 0, 200, 255));
                              dlines->AddLine(
                                  ImVec2(offset.x + (yCanal[0].PR[0] * 20 + 10),
                                         offset.y + 100),
                                  ImVec2(offset.x + (yCanal[0].PR[0] * 20 + 10),
                                         offset.y + 130),
                                  ImColor(0, 0, 200, 255));
                              dlines->AddText(
                                  ImGui::GetFont(), ImGui::GetFontSize(),
                                  ImVec2(offset.x + (yCanal[0].PR[0] * 20 + 2),
                                         offset.y + 120),
                                  ImColor(0, 0, 0, 255), "[PR]", 0, 0.0f, 0);
                           } else if (rotation_cycles == 3) {
                              dlines->AddLine(
                                  ImVec2(offset.x + (yCanal[0].PR[0] * 20 + 5),
                                         offset.y + 100),
                                  ImVec2(offset.x + (yCanal[0].PR[0] * 20 + 5),
                                         offset.y + 130),
                                  ImColor(0, 0, 200, 255));
                              dlines->AddLine(
                                  ImVec2(offset.x + (yCanal[0].PR[0] * 20 + 10),
                                         offset.y + 100),
                                  ImVec2(offset.x + (yCanal[0].PR[0] * 20 + 10),
                                         offset.y + 130),
                                  ImColor(0, 0, 200, 255));
                              dlines->AddText(
                                  ImGui::GetFont(), ImGui::GetFontSize(),
                                  ImVec2(offset.x + (yCanal[0].PR[0] * 20 + 2),
                                         offset.y + 120),
                                  ImColor(0, 0, 0, 255), "[PR]", 0, 0.0f, 0);
                              dlines->AddLine(
                                  ImVec2(offset.x + (yCanal[0].PR[1] * 20 + 5),
                                         offset.y + 100),
                                  ImVec2(offset.x + (yCanal[0].PR[1] * 20 + 5),
                                         offset.y + 130),
                                  ImColor(0, 0, 200, 255));
                              dlines->AddLine(
                                  ImVec2(offset.x + (yCanal[0].PR[1] * 20 + 10),
                                         offset.y + 100),
                                  ImVec2(offset.x + (yCanal[0].PR[1] * 20 + 10),
                                         offset.y + 130),
                                  ImColor(0, 0, 200, 255));
                              dlines->AddText(
                                  ImGui::GetFont(), ImGui::GetFontSize(),
                                  ImVec2(offset.x + (yCanal[0].PR[1] * 20 + 2),
                                         offset.y + 120),
                                  ImColor(0, 0, 0, 255), "[PR]", 0, 0.0f, 0);
                           }
                           if (iCanal[i].KMi == 0) {
                              dlines->AddText(
                                  ImGui::GetFont(), ImGui::GetFontSize(),
                                  ImVec2(offset.x + 10, offset.y + 120),
                                  ImColor(0, 0, 0, 255), AxisValue.c_str(), 0,
                                  0.0f, 0);

                           } else {
                              dlines->AddText(
                                  ImGui::GetFont(), ImGui::GetFontSize(),
                                  ImVec2(offset.x + (iCanal[i].KMi * 20),
                                         offset.y + 80),
                                  ImColor(0, 0, 0, 255), AxisValue.c_str(), 0,
                                  0.0f, 0);
                           }
                           if (yCanal[0].has_dI) {
                              if (i == Kmn - 1) {
                                 dlines->AddText(
                                     ImGui::GetFont(), ImGui::GetFontSize(),
                                     ImVec2(offset.x +
                                                (iCanal[i - 1].KMi * 20 + 20),
                                            offset.y + 60),
                                     ImColor(0, 0, 255, 255), DiAs.c_str(), 0,
                                     0.0f, 0);
                              }
                           }
                        }
                        dlines->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                                        ImVec2(offset.x + 10, offset.y + 160),
                                        ImColor(0, 0, 200, 255),
                                        "PR = Partial Regulator", 0, 0.0f, 0);
                        dlines->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                                        ImVec2(offset.x + 10, offset.y + 180),
                                        ImColor(0, 0, 200, 255),
                                        "DI = Direct Irrigation", 0, 0.0f, 0);
                     } else if (ImGui::Button("Draw")) {
                        drawing = true;
                     };
                     ImGui::EndPopup();
                  }

               } else if (ImGui::Button("Calculate")) {
                  switch (rotation_cycles) {
                     case 2: {
                        auto start = high_resolution_clock::now();
                        rotation(Kmn, cRatio, As, rotation_cycles, FWD, k);
                        auto stop = high_resolution_clock::now();
                        auto duration =
                            duration_cast<milliseconds>(stop - start);
                        elapsed = duration.count();
                        calculation_running = true;

                        break;
                     }

                     case 3: {
                        auto start3 = high_resolution_clock::now();
                        rotation3(Kmn, cRatio, As, rotation_cycles, FWD, k);
                        auto stop3 = high_resolution_clock::now();
                        auto duration3 =
                            duration_cast<milliseconds>(stop3 - start3);
                        elapsed = duration3.count();
                        calculation_running = true;
                        break;
                     }

                     default:
                        break;
                  }
               }

               ImGui::End();
            }

            ImGui::Text(
                "Number of sections %i AS %i rotation cycles %i FWD %i "
                "Factor %f, Z for soil %f",
                Kmn, As, rotation_cycles, FWD, k,dim.Z);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                        1000.0f / ImGui::GetIO().Framerate,
                        ImGui::GetIO().Framerate);
            // ImGui::End();
         }
         ImGui::End();
      }
      // Rendering
      ImGui::Render();
      int display_w, display_h;
      glfwGetFramebufferSize(window, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);
      glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                   clear_color.z * clear_color.w, clear_color.w);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      glfwSwapBuffers(window);
   }

   // Cleanup
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();

   glfwDestroyWindow(window);
   glfwTerminate();

   return 0;
}
