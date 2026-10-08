#pragma once
#include <array>
#include <algorithm>
// Bindings are SDL standard buttons, or one signed half of a standard axis.
// -1 is unbound; 100+axis*2 is positive, 101+axis*2 is negative.
namespace hid::mapping {
inline constexpr std::array<int,24> defaults={0,1,2,3,9,10,4,6,7,8,11,12,13,14,108,110,101,100,103,102,105,104,107,106};
inline bool Valid(int b){return b==-1||(b>=0&&b<21)||(b>=100&&b<112);}
template<class Button,class Axis> int Value(int binding,Button button,Axis axis){
 if(binding>=0&&binding<21)return button(binding)?32767:0;
 if(binding>=100&&binding<112){int n=binding-100,v=axis(n/2);return std::clamp((n&1)?-v:v,0,32767);}
 return 0;
}
inline int ButtonRow(int b){constexpr int rows[]={0,1,2,3,6,-1,7,8,9,4,5,10,11,12,13};return b>=0&&b<15?rows[b]:-1;}
inline int AxisValue(int axis,const std::array<int,24>& values){switch(axis){case 0:return values[17]-values[16];case 1:return values[19]-values[18];case 2:return values[21]-values[20];case 3:return values[23]-values[22];case 4:return values[14];case 5:return values[15];default:return 0;}}
}
