/*********************************************************************************
  *FileName: timezone.h
  *Create Date: 2018/09/21
  *Description: 定义世界个大城市所使用的时区时间与UTC时间差值数组。 
  *Others:  
  *History:  
**********************************************************************************/
#ifndef TIMEZONE_H
#define TIMEZONE_H

#include "typeport.h"


#ifdef __cplusplus
extern "C"
{
#endif
typedef  struct city_time_zone
{
	HLE_S8 		city[32];			//城市
	long 	  	diff_timezone; 		//跟UTC秒数的差距，有正负，范围[-10*3600,14*3600]
	float 		timezone;			//时区(有个别城市的时间使用时区不是整数)
	HLE_U8 		num;				//该对象所处在数组中的下标 
	HLE_S8 		res[3];
} city_time_zone_t ;

city_time_zone_t  tz_diff_table[142] =
{
	{"Honolulu",-36000,-10,0},
	{"Anchorage",-32400,-9,1},
	{"Vancouver",-28800,-8,2},
	{"San Francisco",-28800,-8,3},
	{"Seattle",-28800,-8,4},
	{"Los Angeles",-28800,-8,5},
	{"Aklavik",-25200,-7,6},
	{"Edmonton",-25200,-7,7},
	{"Phoenix",-25200,-7,8},
	{"Denver",-25200,-7,9},
	{"Mexico City",-21600,-6,10},
	{"Winnipeg",-21600,-6,11},
	{"Houston",-21600,-6,12},
	{"Minneapolis",-21600,-6,13},
	{"St.Paul",-21600,-6,14},
	{"New Orleans",-21600,-6,15},
	{"Chicago",-21600,-6,16},
	{"Montgomery",-21600,-6,17},
	{"Guatemala",-21600,-6,18},
	{"San Salvador",-21600,-6,19},
	{"Tegucigalpa",-21600,-6,20},
	{"Managua",-21600,-6,21},
	{"Havana",-18000,-5,22},
	{"Indianapolis",-18000,-5,23},
	{"Atlanta",-18000,-5,24},
	{"Detroit",-18000,-5,25},
	{"Washington DC",-18000,-5,26},
	{"Philadelphia",-18000,-5,27},
	{"Toronto",-18000,-5,28},
	{"Ottawa",-18000,-5,29},
	{"Nassau",-18000,-5,30},
	{"Lima",-18000,-5,31},
	{"Kingston",-18000,-5,32},
	{"Bogota",-18000,-5,33},
	{"New York",-18000,-5,34},
	{"Montreal",-18000,-5,35},
	{"Boston",-18000,-5,36},
	{"Santo Domingo",-18000,-5,37},
	{"La Paz",-18000,-5,38},
	{"Caracas",-18000,-5,39},
	{"San Juan",-18000,-5,40},
	{"Halifax",-18000,-5,41},
	{"Santiago",-14400,-4,42},
	{"Asuncion",-14400,-4,43},
	{"St.John's",-12600,-3.5,44},
	{"Buenos Aires",-10800,-3,45},
	{"Montevideo",-10800,-3,46},
	{"Brasilia",-10800,-3,47},
	{"Sao Paulo",-10800,-3,48},
	{"Rio de Janeiro",-10800,-3,49},
	{"Reykjavik",0,0,50},
	{"Lisbon",0,0,51},
	{"Casablanca",0,0,52},
	{"Dublin",0,0,53},
	{"London",0,0,54},		/*UTC 时间*/
	{"Madrid",3600,+1,55},
	{"Barcelona",3600,+1,56},
	{"Paris",3600,+1,57},
	{"Lagos",3600,+1,58},
	{"Algiers",3600,+1,59},
	{"Brussels",3600,+1,60},
	{"Amsterdam",3600,+1,61},
	{"Geneva",3600,+1,62},
	{"Zurich",3600,+1,63},
	{"Frankfurt",3600,+1,64},
	{"Oslo",3600,+1,65},
	{"Copenhagen",3600,+1,66},
	{"Rome",3600,+1,67},
	{"Berlin",3600,+1,68},
	{"Prague",3600,+1,69},
	{"Zagreb",3600,+1,70},
	{"Vienna",3600,+1,71},
	{"Stockholm",3600,+1,72},
	{"Budapest",3600,+1,73},
	{"Belgrade",3600,+1,74},
	{"Warsaw",3600,+1,75},
	{"Cape Town",7200,+2,76},
	{"Sofia",7200,+2,77},
	{"Athens",7200,+2,78},
	{"Tallinn",7200,+2,79},
	{"Helsinki",7200,+2,80},
	{"Bucharest",7200,+2,81},
	{"Minsk",7200,+2,82},
	{"Johannesburg",7200,+2,83},
	{"Istanbul",7200,+2,84},
	{"Kyiv",7200,+2,85},
	{"Odesa",7200,+2,86},
	{"Harare",7200,+2,87},
	{"Cairo",7200,+2,88},
	{"Ankara",7200,+2,89},
	{"Jerusalem",7200,+2,90},
	{"Beirut",7200,+2,91},
	{"Amman",7200,+2,92},
	{"Khartoum",10800,+3,93},
	{"Nairobi",10800,+3,94},
	{"Moscow",10800,+3,95},
	{"Addis Ababa",10800,+3,96},
	{"Baghdad",10800,+3,97},
	{"Aden",10800,+3,98},
	{"Riyadh",10800,+3,99},
	{"Antananarivo",10800,+3,100},
	{"Kuwait City",10800,+3,101},
	{"Tehran",12600,+3.5,102},
	{"Abu Dhabi",14400,+4,103},
	{"Kabul",16200,+4.5,104},
	{"Karachi",18000,+5,105},
	{"Tashkent",18000,+5,106},
	{"Islamabad",18000,+5,107},
	{"Lahore",18000,+5,108},
	{"Mumbai",19800,+5.5,109},
	{"New Delhi",19800,+5.5,110},
	{"Kolkata",19800,+5.5,111},
	{"Kathmandu",20700,+5.75,112},
	{"Dhaka",21600,+6,113},
	{"Yangon",23400,+6.5,114},
	{"Phnom Penh",25200,+7,115},
	{"Bangkok",25200,+7,116},
	{"Hanoi",25200,+7,117},
	{"Jakarta",25200,+7,118},
	{"Kuala Lumpur",28800,+8,119},
	{"Singapore",28800,+8,120},
	{"Hong Kong",28800,+8,121},
	{"Perth",28800,+8,122},
	{"Beijing",28800,+8,123},
	{"Manila",28800,+8,124},
	{"Shanghai",28800,+8,125},
	{"Taipei",28800,+8,126},
	{"Seoul",32400,+9,127},
	{"Tokyo",32400,+9,128},
	{"Darwin",34200,+9.5,129},
	{"Vladivostok",36000,+10,130},
	{"Brisbane",36000,+10,131},
	{"Melbourne",39600,+11,132},
	{"Canberra",39600,+11,133},
	{"Sydney",39600,+11,134},
	{"Adelaide",41400,+11.5,135},
	{"Kamchatka",43200,+12,136},
	{"Anadyr",43200,+12,137},
	{"Suva",43200,+12,138},
	{"Wellington",46800,+13,139},
	{"Chatham Island",49500,+13.75,140},
	{"Kiritimati",50400,+14,141}
	 /*0~141,共142个城市*/
};

#ifdef __cplusplus
}
#endif
#endif




