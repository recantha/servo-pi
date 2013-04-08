#!/bin/bash

#initialize arrays
min[0]=50
min[1]=50
min[2]=50
min[3]=50
min[4]=50
min[5]=50
min[6]=50
min[7]=50

max[0]=250
max[1]=250
max[2]=250
max[3]=250
max[4]=250
max[5]=250
max[6]=250
max[7]=250

neutral[0]=150
neutral[1]=150
neutral[2]=150
neutral[3]=150
neutral[4]=150
neutral[5]=150
neutral[6]=150
neutral[7]=150

inc[0]=10
inc[1]=10
inc[2]=10
inc[3]=10
inc[4]=10
inc[5]=10
inc[6]=10
inc[7]=10

red='\e[0;31m'
endColor='\e[0m'
active=0

#define functions

#display current pulse width values with active servo highlited
function prn_curr_pw {
active=$1
echo $active=${pw[$active]} > /dev/servoblaster
clear
for index in {0..7}
do
  if [ $index = $active ]; then
    echo -e ${red}$index":"${pw[$index]}0${endColor}" \c" 
  else
    echo -e $index":"${pw[$index]}"0 \c" 
  fi
done
echo " "
echo Servo $active pulse width increment ${inc[$active]}0 uS.
}

#main program
sudo sbload
clear
pw[0]=${neutral[0]}
echo 0=${pw[0]} > /dev/servoblaster
echo -e ${red}"0:"${pw[$active]}0${endColor}" \c"
for index in {1..7}
do
  pw[$index]=${neutral[$index]}         #set pulse width to neutral values
  echo $index=${pw[$index]} > /dev/servoblaster
  echo -e $index":"${pw[$index]}"0 \c"   #display initial values
done
echo " "
echo Servo $active pulse width increment ${inc[0]}0 uS.
while true            #read keypresses and act
do
  read -sn1 a
  case "$a" in            #number keypress sets active servo
    0)  prn_curr_pw $a
        active=$a;;
    1)  prn_curr_pw $a
        active=$a;;
    2)  prn_curr_pw $a
        active=$a;;
    3)  prn_curr_pw $a
        active=$a;;
    4)  prn_curr_pw $a
        active=$a;;
    5)  prn_curr_pw $a
        active=$a;;
    6)  prn_curr_pw $a
        active=$a;;
    7)  prn_curr_pw $a
        active=$a;;
    x)  clear
   for index in {0..7}
   do
       pw[$index]=${neutral[$index]}         #set pulse width to neutral values
       echo $index=${pw[$index]} > /dev/servoblaster
       echo -e $index":"${pw[$index]}"0 \c"   #display final values
   done
   echo " "   
   sudo sbunload
   exit
  esac
  test "$a" == `echo -en "\e"` || continue
  read -sn1 a
  test "$a" == "[" || continue
  read -sn1 a
  case "$a" in
    A)  let "new=${pw[$active]}+${inc[$active]}"    #keypress up arrow
   if [ "$new" -le "${max[$active]}" ]; then   #increase width
     let "pw[$active]=$new"
   fi
        prn_curr_pw $active;;
    B)  let "new=${pw[$active]}-${inc[$active]}"   #keypress up arrow
   if [ "$new" -ge "${min[$active]}" ]; then   #decrease width
     let "pw[$active]=$new"
   fi
        prn_curr_pw $active;;
    C)  let "active=$active+1"            #keypress left arrow
   if [ "$active" -eq 8 ]; then         #previous servo number
     active=0
   fi
   prn_curr_pw $active;;
    D)  let "active=$active-1"            #keypress right arrow
   if [ "$active" -eq -1 ]; then         #next servo number
     active=7
   fi
   prn_curr_pw $active;;
    5)  let "inc[$active]=${inc[$active]}+1"      #keypress pageup
   prn_curr_pw $active;;            #increase step
    6)  let "new=${inc[$active]}-1"         #keypress pageup
   if [ "$new" -ge 1 ]; then         #decrease width
     let "inc[$active]=$new"
   fi
        prn_curr_pw $active;;
  esac
done

