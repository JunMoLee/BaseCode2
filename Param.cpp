/*******************************************************************************
* Copyright (c) 2015-2017
* School of Electrical, Computer and Energy Engineering, Arizona State University
* PI: Prof. Shimeng Yu
* All rights reserved.
*   
* This source code is part of NeuroSim - a device-circuit-algorithm framework to benchmark 
* neuro-inspired architectures with synaptic devices(e.g., SRAM and emerging non-volatile memory). 
* Copyright of the model is maintained by the developers, and the model is distributed under 
* the terms of the Creative Commons Attribution-NonCommercial 4.0 International Public License 
* http://creativecommons.org/licenses/by-nc/4.0/legalcode.
* The source code is free and you can redistribute and/or modify it
* by providing that the following conditions are met:
*   
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer. 
*   
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*   
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* Developer list: 
*   Pai-Yu Chen     Email: pchen72 at asu dot edu 
*                     
*   Xiaochen Peng   Email: xpeng15 at asu dot edu
********************************************************************************/
#include <string>
#include "math.h"
#include "Param.h"

Param::Param() {
	/* MNIST dataset */
	numMnistTrainImages = 60000;// # of training images in MNIST
	numMnistTestImages = 10000;	// # of testing images in MNIST
	useLTD=1;
	const double
        l=30;
	const double
	ll=30;
	const double
	dl=30;
	const double
	pdl=30;
	const double
	rdl=10;
	const double
	ss=0;
	selectsim=ss;
	
	/* Weight track variables */
	weighttrack=0;
	TrackRate=500;
	track=0;
	Gth1=2;
	Gth2=8;
	
	/* Algorithm parameters */
	numTrainImagesPerEpoch = 1998;	// # of training images per epoch
	totalNumEpochs = 125;	// Total number of epochs
	interNumEpochs = 1;		// Internal number of epochs (print out the results every interNumEpochs)
	nInput = 400;     // # of neurons in input layer
	nHide = 100;      // # of neurons in hidden layer
	nOutput = 10;     // # of neurons in output layer
	
	/* learning rate */
	
	speed = 2; 
	alpha1 = l/100.0;	// Learning rate for the weights from input to hidden layer
	alpha2 = alpha1/2;	// Learning rate for the weights from hidden to output layer
	
	dalpha = dl/100;        // LTD learning rate (-)
	pdalpha = pdl/100;      // LTD learning rate (+)
	
	nalpha1 = l/100;        // LTP learning rate (-) for IH weights
	nalpha2 = l/2.0/100;    // LTP learning rate (-) for HO weights
	
	LTPIHasf = nalpha1/alpha1;                    // asymmetry factor of LTD curve for IH defined by (-)/(+)
	LTPHOasf = (nalpha1/speed)/(alpha1/speed);                // asymmetry factor of LTD curve for IH defined by (-)/(+)
	LTDIHasf = dalpha/pdalpha;                // asymmetry factor of LTD curve for IH defined by (-)/(+)
	LTDHOasf = (dalpha/speed)/(pdalpha/speed);                  // asymmetry factor of LTD curve for HO defined by (-)/(+)
	
	defaultlearningrate[0][0]= alpha1;
	defaultlearningrate[0][1]= nalpha1;
	defaultlearningrate[0][2]= pdalpha;
	defaultlearningrate[0][3]= dalpha;

	/* setting learning rate for designated areas */
	learningrate[0][0] = alpha1;
	learningrate[0][1] = nalpha1;
	learningrate[0][2] = pdalpha;
	learningrate[0][3] = dalpha;
		
	learningrate[1][0] = alpha1;
	learningrate[1][1] = nalpha1;
	learningrate[1][2] = pdalpha;
	learningrate[1][3] = dalpha;
		
	tp = 12.5;
	tn = 12.5;
	kp=128;
	kd=128;
	knp=97;
	knd=97;
	kc=64;
	pp= 1;
	pd=-1; 
	np=1;
	nd=-1;
	cp=1;
	cp=-1;
	const int
	dom=0;
	dominance=dom;
	maxWeight = 1;	// Upper bound of weight value
	minWeight = -1;	// Lower bound of weight value
    /*Optimization method 
    Available option include: "SGD", "Momentum", "Adagrad", "RMSprop" and "Adam"*/
    optimization_type = "SGD";
	const int
	nuc=1;
        ReverseUpdate = nuc;
	const int 
        frc=0;
	FullRefresh= frc;

	/* Hardware parameters */
	useHardwareInTrainingFF = true;   // Use hardware in the feed forward part of training or not (true: realistic hardware, false: ideal software)
	useHardwareInTrainingWU = true;   // Use hardware in the weight update part of training or not (true: realistic hardware, false: ideal software)
	useHardwareInTraining = useHardwareInTrainingFF || useHardwareInTrainingWU;    // Use hardware in the training or not
	useHardwareInTestingFF = true;    // Use hardware in the feed forward part of testing or not (true: realistic hardware, false: ideal software)
	numBitInput = 1;       // # of bits of the input data (=1 for black and white data)
	numBitPartialSum = 8;  // # of bits of the digital output (partial weighted sum output)
	pSumMaxHardware = pow(2, numBitPartialSum) - 1;   // Max digital output value of partial weighted sum
	numInputLevel = pow(2, numBitInput);  // # of levels of the input data
	numWeightBit = 6;	// # of weight bits (only for pure algorithm, SRAM and digital RRAM hardware)
	BWthreshold = 0.5;	// The black and white threshold for numBitInput=1
	Hthreshold = 0.5;	// The spiking threshold for the hidden layer (da1 in Train.cpp and Test.cpp)
	numColMuxed = 16;	// How many columns share 1 read circuit (for analog RRAM) or 1 S/A (for digital RRAM)
	numWriteColMuxed = 16;	// How many columns share 1 write column decoder driver (for digital RRAM)
	writeEnergyReport = true;	// Report write energy calculation or not
	NeuroSimDynamicPerformance = true; // Report the dynamic performance (latency and energy) in NeuroSim or not
	relaxArrayCellHeight = 0;	// True: relax the array cell height to standard logic cell height in the synaptic array
	relaxArrayCellWidth = 0;	// True: relax the array cell width to standard logic cell width in the synaptic array
	arrayWireWidth = 100;	// Array wire width (nm)
	processNode = 32;	// Technology node (nm)
	clkFreq = 2e9;		// Clock frequency (Hz)
        const int 
	frr=1000;
	RefreshRate = frr;
	const int 
	nur=2;
	const int 
	rnur=1;
	const int
	nurn=2;
	newUpdateRate = nur; // rate of new update algorithm implementation (per # of images)
	nnewUpdateRate =nurn;
const int
a=1;
	

switch(a){
case 0:
param_gp=1;
param_gn=-9;
break;
case 1:
param_gp=1;
param_gn=-3;
break;
case 2:
param_gp=1;
param_gn=-1;
break;
case 3:
param_gp=0;
param_gn=-9;
break;
case 4:
param_gp=4;
param_gn=-7;
break;
case 5:
param_gp=3;
param_gn=-3;
break;
case 6:
param_gp=3;
param_gn=-1;
break;
case 7:
param_gp=9;
param_gn=-1;
break;
case 8:
param_gp=8;
param_gn=-1;
break;
case 9:
param_gp=5;
param_gn=1;
break;
case 10:
param_gp=5;
param_gn=-3;
break;
case 11:
param_gp=1;
param_gn=-8;
break;
}
	
	
}

void Param::ChangeLearningrate(double alph1, double nalph, double dalph, double pdalph){
	this->alpha1=alph1;
	this->dalpha=dalph;
	this->nalpha1=nalph;
	this->pdalpha=pdalph;
}

void Param::ChangeNur(double nurn, double rnur){
	this->newUpdateRate=(int)rnur*nurn;
	this->nnewUpdateRate=(int)nurn;
}

