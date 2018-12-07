#!/bin/sh

SOURCE_PATH=./     
DEST_PATH=./

h_c_to_hh_cc=0  #just use  at vmware_share 

hh_cc_to_h_c=1  #vmware_share(windows) to  svn_debug(linux)  

echo SOURCE_PATH = $SOURCE_PATH
echo DEST_PATH = $DEST_PATH


######将 .h  .c .cpp 文件转换成  .hh  .cc .cppp文件################################
if test $[h_c_to_hh_cc] -eq 1 
then
	c_file=`find $SOURCE_PATH -name *.c`
	h_file=`find $SOURCE_PATH -name *.h`
	cpp_file=`find $SOURCE_PATH -name *.cpp`

	for file in $c_file		# .c  --->  .cc
	do
		new_file=${file%.*}		#%.* 表示从右边开始，删除第一个 . 号及右边的字符
		new_file=${new_file}.cc
		#echo $new_file
		mv $file $new_file 
		echo "mv $file $new_file" 

	done

	for file in $h_file		# .h  --->  .hh
	do
		new_file=${file%.*}		#%.* 表示从右边开始，删除第一个 . 号及右边的字符
		new_file=${new_file}.hh
		#echo $new_file
		mv $file $new_file 
		echo "mv $file $new_file" 

	done

	for file in $cpp_file    #.cpp ---> .cppp
	do
		new_file=${file%.*}		#%.* 表示从右边开始，删除第一个 . 号及右边的字符
		new_file=${new_file}.cppp
		#echo $new_file
		mv $file $new_file 
		echo "mv $file $new_file" 
	done

fi
#####################################################################


######将 .hh  .c .cppp文件拷贝到指定目录下并转换成  .h  .c  .cpp文件####################
if test $[hh_cc_to_h_c] -eq 1 
then
	#拷贝
	#shopt -s  extglob    			#开启扩展通配符
	#rm -rf $DEST_PATH/!(libcurl) 	#删除除了libcurl的文件
	# cd $DEST_PATH
	# rm_files=`find  ./-name "libcurl"` 
	# echo $rm_files
	# cd -

	# exit 0

	# cp -r $SOURCE_PATH/[^libcurl]* $DEST_PATH/

	#rm -rf $DEST_PATH/*
	#cp -rf $SOURCE_PATH/* $DEST_PATH/
	#带链接的拷贝
	#cp -rdpf $SOURCE_PATH/* $DEST_PATH/   





	#转换
	cc_file=`find $DEST_PATH -name '*.cpp'`
	hh_file=`find $DEST_PATH -name '*.hh'`
	cppp_file=`find $DEST_PATH -name '*.cppp'`

	for file in $cc_file		# .cc  --->  .c
	do
		new_file=${file%.*}		#%.* 表示从右边开始，删除第一个 . 号及右边的字符
		new_file=${new_file}.c
		#echo $new_file
		mv $file $new_file 
		echo "mv $file $new_file" 

	done

	for file in $hh_file		# .hh  --->  .h
	do
		new_file=${file%.*}		#%.* 表示从右边开始，删除第一个 . 号及右边的字符
		new_file=${new_file}.h
		#echo $new_file
		mv $file $new_file 
		echo "mv $file $new_file" 

	done

	for file in $cppp_file		# .cppp  --->  .cpp
	do
		new_file=${file%.*}		#%.* 表示从右边开始，删除第一个 . 号及右边的字符
		new_file=${new_file}.cpp
		#echo $new_file
		mv $file $new_file 
		echo "mv $file $new_file" 

	done
fi
#####################################################################



# function read_dir(){
#     for file in `ls $1`			#$1表示函数第一个参数，$2表示第二个参数...
#     do
#         if [ -d $file ] 
#         then					#递归调用 read_dir
#         	cd ./$file
#         	for file1 in `ls ../$file`
#         	do
#         		echo "pwd = `pwd`"
#         		echo "read_dir $file1"
#         		read_dir $file1

#         	done
#         	cd ..
#         else					#else is a file
# 			if [ "${file##*.}"x = "$TAIL1"x ]
# 			then
# 				main_name=`ls $file | cut -d . -f 1`
# 				new_file_name=${main_name}.${TAIL1_TO}
# 				echo "mv $file to $new_file_name"
# 				#mv $file $new_file_name
# 			fi

# 			if [ "${file##*.}"x = "$TAIL2"x ]
# 			then
# 				main_name=`ls $file|cut -d . -f 1`
# 				new_file_name=${main_name}.${TAIL2_TO}
# 				#echo "mv $file to $new_file_name"
# 				#mv $file $new_file_name
# 			fi
			
# 			#echo " $file is unknown file type!"

# 		fi

#     done
# }


# read_dir $SOURCE_PATH


