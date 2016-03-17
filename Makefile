# C++ Compiler
CC=g++ -std=c++14

# Using Debug flags
CFLAGS=-Wall -O3

# Using for clearer error finding
BONUS=2>&1 | grep -E --color=always 'error|warning|$$'

# For OpenCV support
CFLAGS+=`pkg-config --cflags opencv` -fopenmp
LDFLAGS+=`pkg-config --libs opencv` -L/usr/local/cuda-7.5/lib64 -lboost_system -lboost_timer -lgomp -lglut -lGL -lGLEW -lGLU  -pthread

# Paths
SRC=src/core/src
OBJ=obj
SRC_FILES = $(wildcard $(SRC)/*.cpp)
DEPD = $(wildcard $(SRC)/header/.h)
BIN=bin
DATA=data/mp4
INC=-I/usr/local/cuda-7.5/extras/CUPTI/include -I$(SRC) -I/usr/local/cuda-7.5/targets/x86_64-linux/include/ 

# All files
OBJ_FILES := $(addprefix obj/,$(notdir $(SRC_FILES:.cpp=.o)))

# Used for debug
cccyan=$(shell echo "\033[0;36m")
ccend=$(shell echo "\033[0m")

all: VideoStitch

tools: VideoStitch CameraCalibrator ImagesDumper

BUILD_PRINT = \e[1;34mBuilding $<\e[0m

debug:
	objdump -d $(BIN)/VideoStitch > dump

makeObj: $(OBJ_FILES)
	@echo "$(cccyan)[Obj files generated]$(ccend)"

$(OBJ)/%.o: $(SRC)/%.cpp $(SRC)/header/%.h
	@mkdir -p $(OBJ)
	@echo "$(cccyan)[Run OBJ $@ compile]$(ccend)"	
	$(CC) $(CFLAGS) $(INC) -c -o $@ $< 

VideoStitch: $(OBJ_FILES) 
	@echo "$(cccyan)[Run Link compile]$(ccend)"
	$(CC) $? -I$(SRC) src/cmd/main.cpp -o $(BIN)/$@ $(LDFLAGS)

socket: $(OBJ_FILES) 
	$(CC) $? -I$(SRC) -I. src/cardboard/VSSocket.cpp src/cardboard/server.cpp -o $(BIN)/$@ $(LDFLAGS)

socketC:
	$(CC) obj/Usage.o $? -I$(SRC) src/cardboard/client.cpp -o $(BIN)/$@ $(LDFLAGS)

bauzi: 
	$(CC) obj/Usage.o $? -I$(SRC) -Itools/BauziCalibration tools/BauziCalibration/BauziCalibrator.cpp -o $(BIN)/$@ $(LDFLAGS)

featureGenerator: 
	$(CC) obj/Usage.o $? -I$(SRC) -Itools/ManualFeatureGenerator tools/BauziCalibration/ManualFeatureGenerator.cpp -o $(BIN)/$@ $(LDFLAGS)

PR: $(OBJ_FILES)
	@echo "$(cccyan)[Run Link compile]$(ccend)"
	$(CC) $? -I$(SRC) src/opengl/PartialRenderGUI.cpp -o $(BIN)/$@ $(LDFLAGS)

CameraCalibrator:
	@echo "$(cccyan)[Ganerate camera calibrator]$(ccend)"
	$(CC) -o $(BIN)/$@ tools/calibration/CameraCalibrator.cpp $(LDFLAGS)

ImagesDumper:
	@echo "$(cccyan)[Ganerate images dumper]$(ccend)"
	$(CC) -I$(SRC) obj/Usage.o -o $(BIN)/$@ tools/imagesDumper/ImagesDumper.cpp $(LDFLAGS)

run:
	#$(BIN)/VideoStitch --input data/gopro/inputVideo.txt --calibration data/MultiCalibration/calibrationResult.txt --pto data/Cut15/15.pto --duration 100 --output StitchResult.avi
	#$(BIN)/VideoStitch --input data/MultiCalibration/inputVideo.txt --calibration data/MultiCalibration/calibrationResult.txt --pto data/Cut15/15.pto --duration 100 --output StitchResult.avi
	$(BIN)/VideoStitch --input data/Cut15/inputVideo.txt --calibration data/Cut15/Calibration.txt --pto data/Cut15/15.pto --duration 100 --output StitchResult.avi

runPR:
	$(BIN)/PR --input data/Cut15/inputVideo.txt --calibration data/Cut15/Calibration.txt --pto data/Cut15/15.pto --duration 300 --output StitchResult.avi

runBauzi:
	$(BIN)/bauzi --input data/Bauzi/inputImage.txt --iter 10 --featureInfo FeatureInfo.txt

runFeatureGenerator:
	$(BIN)/featureGenerator --input data/Bauzi/inputImage.txt

runSocket:
	$(BIN)/socket --input data/Cut15/inputVideo.txt --calibration data/Cut15/Calibration.txt --pto data/Cut15/15.pto --duration 100 --output StitchResult.avi

calibrator:
	$(BIN)/CameraCalibrator data/CalibrationImages2/input_config.xml

dumper:
	$(BIN)/ImagesDumper data/Bauzi/inputVideo.txt data/Library20160216/pattern.png 0 1 1 data/Bauzi/raw/
	#$(BIN)/ImagesDumper data/Library20160216/inputVideo2.txt data/Library20160216/pattern.png 0 1 1 data/Library20160216/dump4
	#$(BIN)/ImagesDumper data/Library20160216/inputVideo2.txt data/Library20160216/pattern.png 0 1 1 data/Library20160216/dump3

clean:
	- rm -r $(BIN)/VideoStitch
	- rm -rf obj/*.o

CC_FLAGS += -MMD
-include $(OBJFILES:.o=.d)
