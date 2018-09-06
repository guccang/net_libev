#把所有的目录都做成变量,方便修改和移植
BIN=./bin
SRC=$2
SRCCC=./pb
INC=-I./include -I./pb
OBJ=./obj

#提前所有源文件 (*.cpp) 和所有中间文件(*.o)
SOURCE=${wildcard ${SRC}/*.cpp}
SOURCECC=${wildcard ${SRCCC}/*.cc}

OBJECT=${patsubst %.cpp,${OBJ}/%.o,${notdir ${SOURCE}}}
OBJECT+=${patsubst %.cc,${OBJ}/%.o,${notdir ${SOURCECC}}}

#test:
#	echo $(SROUCECC)
#	echo $(SROUCE)
#	echo $(OBJECT)

#设置最后目标文件
TARGET=$1
BIN_TARGET=${BIN}/${TARGET}

CC=g++
CFLAGS=-g -Wall ${INC}

#用所有中间文件生成目标文件,规则中可以用$^替换掉${OBJECT}
${BIN_TARGET}:${OBJECT}
	$(CC) -lev -lprotobuf -o $@ ${OBJECT}

${OBJ}/%.o:${SRC}/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

${OBJ}/%.o:${SRCCC}/%.cc
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY:clean
clean:
	find $(OBJ) -name *.o | xargs rm -rf
	rm -rf $(BIN)/*
