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

#include <cstdio>
#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <fstream>
#include "formula.h"
#include "Param.h"
#include "Array.h"
#include "Mapping.h"
#include "NeuroSim.h"


using namespace std;

extern Param *param;

extern std::vector< std::vector<double> > Input;
extern std::vector< std::vector<int> > dInput;
extern std::vector< std::vector<double> > Output;

extern std::vector< std::vector<double> > weight1;
extern std::vector< std::vector<double> > weight2;
extern std::vector< std::vector<double> > deltaWeight1;
extern std::vector< std::vector<double> > deltaWeight2;
extern std::vector< std::vector<double> >  totalDeltaWeight1;
extern std::vector< std::vector<double> >  totalDeltaWeight1_abs;
extern std::vector< std::vector<double> >  totalDeltaWeight2;
extern std::vector< std::vector<double> >  totalDeltaWeight2_abs;

extern std::vector< std::vector<double> >  gradSquarePrev1;
extern std::vector< std::vector<double> >  gradSquarePrev2;
extern std::vector< std::vector<double> >  momentumPrev1;
extern std::vector< std::vector<double> >  momentumPrev2;
extern std::vector< std::vector<double> >  gradSum1;
extern std::vector< std::vector<double> >  gradSum2;


extern Technology techIH;
extern Technology techHO;
extern Array *arrayIH;
extern Array *arrayHO;
extern SubArray *subArrayIH;
extern SubArray *subArrayHO;
extern Adder adderIH;
extern Mux muxIH;
extern RowDecoder muxDecoderIH;
extern DFF dffIH;
extern Subtractor subtractorIH;
extern Adder adderHO;
extern Mux muxHO;
extern RowDecoder muxDecoderHO;
extern DFF dffHO;
extern Subtractor subtractorHO;

extern double totalWeightUpdate=0; // track the total weight update (absolute value) during the whole training process
extern double totalNumPulse=0;// track the total number of pulse for the weight update process; for Analog device only


/*Weight track variables */
			        vector <double> prevpossatsum2(220,0);
                                vector <double> prevnegsatsum2(220,0);
				vector <double> prevposstepcount2(220,0);
				vector <double> prevnegstepcount2(220,0);
				/* double prevpossigcount2=0, prevnegsigcount2=0; */
				vector <double> prevweightsum2(220,0);
				/* double prevzerosigcount2=0; */

			        vector <double> prevpossatsum1(220,0);
                                vector <double> prevnegsatsum1(220,0);
				vector <double> prevposstepcount1(220,0);
				vector <double> prevnegstepcount1(220,0);
				/* double prevpossigcount1=0, prevnegsigcount1=0; */
				vector <double> prevweightsum1(220,0);
				/* double prevzerosigcount1=0; */



				
/*Optimization functions*/
double gradt;
double GAMA=0.9;
double BETA1= 0.1, BETA2=0.7; 
double SGD(double gradient, double learning_rate);
double Momentum(double gradient, double learning_rate, double momentumPrev, double GAMA=0.1);
double Adagrad(double gradient, double learning_rate, double gradSquare, double EPSILON=1E-1);
double RMSprop(double gradient, double learning_rate, double gradSquarePrev,double GAMA=0.5, double EPSILON=2E-1);
double Adam(double gradient, double learning_rate, double momentumPreV, double velocityPrev, double BETA1=0.1, double BETA2=0.7, double EPSILON=2E-1);


void Train(const int numTrain, const int epochs, char *optimization_type, int epochcount, bool stopreset, bool stopreverse, double adLA, double adFrr, double adNur) {
int numBatchReadSynapse;	    // # of read synapses in a batch read operation (decide later)
int numBatchWriteSynapse;	// # of write synapses in a batch write operation (decide later)
double outN1[param->nHide]; // Net input to the hidden layer [param->nHide]
double a1[param->nHide];    // Net output of hidden layer [param->nHide] also the input of hidden layer to output layer
                                // the value after the activation function
                                // also the input of hidden layer to output layer
int da1[param->nHide];  // Digitized net output of hidden layer [param->nHide] also the input of hidden layer to output layer
double outN2[param->nOutput];   // Net input to the output layer [param->nOutput]
double a2[param->nOutput];  // Net output of output layer [param->nOutput]

double s1[param->nHide];    // Output delta from input layer to the hidden layer [param->nHide]
double s2[param->nOutput];  // Output delta from hidden layer to the output layer [param->nOutput]
	
double Gth1= param->Gth1;
double Gth2= param->Gth2;
	
	for (int t = 0; t < epochs; t++) {
		for (int batchSize = 0; batchSize < numTrain; batchSize++) {

			int i = rand() % param->numMnistTrainImages;  // Randomize sample
            //int i = 1;       // use this value for debug
			// Forward propagation
			/* First layer (input layer to the hidden layer) */
			std::fill_n(outN1, param->nHide, 0);
			std::fill_n(a1, param->nHide, 0);
        if (param->useHardwareInTrainingFF) {   // Hardware
				double sumArrayReadEnergy = 0;   // Use a temporary variable here since OpenMP does not support reduction on class member
                double readVoltage; 
                double readPulseWidth;

           if(AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayIH->cell[0][0]))
           {
                 //printf("This is AnalogNVM\n");
                 readVoltage = static_cast<eNVM*>(arrayIH->cell[0][0])->readVoltage;
				 readPulseWidth = static_cast<eNVM*>(arrayIH->cell[0][0])->readPulseWidth;
           }
            #pragma omp parallel for reduction(+: sumArrayReadEnergy)
				for (int j=0; j<param->nHide; j++) {
					if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayIH->cell[0][0])) {  // Analog eNVM
						if (static_cast<eNVM*>(arrayIH->cell[0][0])->cmosAccess) {  // 1T1R
							sumArrayReadEnergy += arrayIH->wireGateCapRow * techIH.vdd * techIH.vdd * param->nInput; // All WLs open
						}
					} else if (DigitalNVM *temp = dynamic_cast<DigitalNVM*>(arrayIH->cell[0][0])) { // Digital eNVM
						if (static_cast<eNVM*>(arrayIH->cell[0][0])->cmosAccess) {  // 1T1R
							sumArrayReadEnergy += arrayIH->wireGateCapRow * techIH.vdd * techIH.vdd; // Selected WL
						} else {    // Cross-point
							sumArrayReadEnergy += arrayIH->wireCapRow * techIH.vdd * techIH.vdd * (param->nInput - 1);  // Unselected WLs
						}
					}
                    
					for (int n=0; n<param->numBitInput; n++) {
						double pSumMaxAlgorithm = pow(2, n) / (param->numInputLevel - 1) * arrayIH->arrayRowSize;  // Max algorithm partial weighted sum for the nth vector bit (if both max input value and max weight are 1)
						if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayIH->cell[0][0])) {  // Analog eNVM
                            //printf("calculating the current sum\n");
							double Isum = 0;    // weighted sum current
							double IsumMax = 0; // Max weighted sum current
              double IsumMin = 0; 
							double inputSum = 0;    // Weighted sum current of input vector * weight=1 column
							for (int k=0; k<param->nInput; k++) {
								if ((dInput[i][k]>>n) & 1) {    // if the nth bit of dInput[i][k] is 1
									Isum += arrayIH->ReadCell(j,k);
                                    inputSum += arrayIH->GetMediumCellReadCurrent(j,k);    // get current of Dummy Column as reference
									sumArrayReadEnergy += arrayIH->wireCapRow * readVoltage * readVoltage; // Selected BLs (1T1R) or Selected WLs (cross-point)
								}
								IsumMax += arrayIH->GetMaxCellReadCurrent(j,k);
                IsumMin += arrayIH->GetMinCellReadCurrent(j,k);
							}
							sumArrayReadEnergy += Isum * readVoltage * readPulseWidth;
							int outputDigits = 2*(CurrentToDigits(Isum, IsumMax-IsumMin)-CurrentToDigits(inputSum, IsumMax-IsumMin));   
                            outN1[j] += DigitsToAlgorithm(outputDigits, pSumMaxAlgorithm);
						}
                        else 
                        {    // SRAM or digital eNVM
                            bool digitalNVM = false; 
                            bool parallelRead = false;
                            if(DigitalNVM*temp = dynamic_cast<DigitalNVM*>(arrayIH->cell[0][0]))
                            {    digitalNVM = true;
                                if(static_cast<DigitalNVM*>(arrayIH->cell[0][0])->parallelRead == true) 
								{
                                    parallelRead = true;
                                }
                            }
                            if(digitalNVM && parallelRead) // parallel read-out for DigitalNVM
                            {
                                    //printf("This is parallel read-out\n");
                                    double Imax = static_cast<DigitalNVM*>(arrayIH->cell[0][0])->avgMaxConductance*static_cast<DigitalNVM*>(arrayIH->cell[0][0])->readVoltage;
                                    double Imin = static_cast<DigitalNVM*>(arrayIH->cell[0][0])->avgMinConductance*static_cast<DigitalNVM*>(arrayIH->cell[0][0])->readVoltage;
                                    double Isum = 0;    // weighted sum current
							        double IsumMax = 0; // Max weighted sum current
							        double inputSum = 0;    // Weighted sum current of input vector * weight=1 column
                                    int Dsum=0;
                                    int DsumMax = 0;
                                    int Dref = 0;
                                    for (int w=0;w<param->numWeightBit;w++){
                                        int colIndex = (j+1) * param->numWeightBit - (w+1);  // w=0 is the LSB
									    for (int k=0; k<param->nInput; k++) 
                                        {
										    if((dInput[i][k]>>n) & 1){ // accumulate the current along a column
											    Isum += static_cast<DigitalNVM*>(arrayIH->cell[colIndex][k])->conductance*static_cast<DigitalNVM*>(arrayIH->cell[colIndex ][k])->readVoltage;
                                                inputSum += static_cast<DigitalNVM*>(arrayIH->cell[arrayIH->refColumnNumber][k])->conductance*static_cast<DigitalNVM*>(arrayIH->cell[arrayIH->refColumnNumber][k])->readVoltage;
										    }
									    }

                                        int outputDigits = (int) (Isum /(Imax-Imin)); // the output at the ADC of this column // basically, this is the number of "1" in the partial sum of this column                                                                                                               
                                        int outputDigitsRef = (int) (inputSum/(Imax-Imin));
                                        outputDigits = outputDigits-outputDigitsRef;
                                        
                                        Dref = (int)(inputSum/Imin);
                                        Isum=0;
                                        inputSum=0;
                                        Dsum += outputDigits*(int) pow(2,w);  // get the weight represented by the column
                                        DsumMax += param->nInput*(int) pow(2,w); // the maximum weight that can be represented by this column
                                    }
                                    sumArrayReadEnergy += static_cast<DigitalNVM*>(arrayHO->cell[0][0])->readEnergy * arrayHO->numCellPerSynapse * arrayHO->arrayRowSize;
                                    outN1[j] += (double)(Dsum - Dref*(pow(2,param->numWeightBit-1)-1)) / DsumMax * pSumMaxAlgorithm;
                            }
                            else
                            {	 // Digital NVM or SRAM row-by-row readout				
							    int Dsum = 0;
							    int DsumMax = 0;
							    int inputSum = 0;
							    for (int k=0; k<param->nInput; k++) {
								    if ((dInput[i][k]>>n) & 1) {    // if the nth bit of dInput[i][k] is 1
									    Dsum += (int)(arrayIH->ReadCell(j,k));
									    inputSum += pow(2, arrayIH->numCellPerSynapse-1) - 1;   // get the digital weights of the dummy column as reference
								    }
								    DsumMax += pow(2, arrayIH->numCellPerSynapse) - 1;
							    }
							    if (DigitalNVM *temp = dynamic_cast<DigitalNVM*>(arrayIH->cell[0][0])) {    // Digital eNVM
								    sumArrayReadEnergy += static_cast<DigitalNVM*>(arrayIH->cell[0][0])->readEnergy * arrayIH->numCellPerSynapse * arrayIH->arrayRowSize;
							    } 
                                else {    // SRAM
								    sumArrayReadEnergy += static_cast<SRAM*>(arrayIH->cell[0][0])->readEnergy * arrayIH->numCellPerSynapse * arrayIH->arrayRowSize;
							    }
							    outN1[j] += (double)(Dsum - inputSum) / DsumMax * pSumMaxAlgorithm;
							}
						}
					}
					a1[j] = sigmoid(outN1[j]);
					da1[j] = round_th(a1[j]*(param->numInputLevel-1), param->Hthreshold);
				}
				arrayIH->readEnergy += sumArrayReadEnergy;

				numBatchReadSynapse = (int)ceil((double)param->nHide/param->numColMuxed);
				// Don't parallelize this loop since there may be update of member variables inside NeuroSim functions
				for (int j=0; j<param->nHide; j+=numBatchReadSynapse) {
					int numActiveRows = 0;  // Number of selected rows for NeuroSim
					for (int n=0; n<param->numBitInput; n++) {
						for (int k=0; k<param->nInput; k++) {
							if ((dInput[i][k]>>n) & 1) {    // if the nth bit of dInput[i][k] is 1
								numActiveRows++;
							}
						}
					}
					subArrayIH->activityRowRead = (double)numActiveRows/param->nInput/param->numBitInput;
					subArrayIH->readDynamicEnergy += NeuroSimSubArrayReadEnergy(subArrayIH);
					subArrayIH->readDynamicEnergy += NeuroSimNeuronReadEnergy(subArrayIH, adderIH, muxIH, muxDecoderIH, dffIH, subtractorIH);
					subArrayIH->readLatency += NeuroSimSubArrayReadLatency(subArrayIH);
					subArrayIH->readLatency += NeuroSimNeuronReadLatency(subArrayIH, adderIH, muxIH, muxDecoderIH, dffIH, subtractorIH);
				}
        } 
        else {    // Algorithm
				#pragma omp parallel for
				for (int j = 0; j < param->nHide; j++) {
					for (int k = 0; k < param->nInput; k++) {
						outN1[j] += Input[i][k] * weight1[j][k];
					}
					a1[j] = sigmoid(outN1[j]);
				}
        }

			/* Second layer (hidder layer to the output layer) */
			std::fill_n(outN2, param->nOutput, 0);
			std::fill_n(a2, param->nOutput, 0);
			if (param->useHardwareInTrainingFF) {   // Hardware
            double sumArrayReadEnergy = 0;  // Use a temporary variable here since OpenMP does not support reduction on class member
            double readVoltage;
            double readPulseWidth;
            if(AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayHO->cell[0][0])){
                readVoltage = static_cast<eNVM*>(arrayHO->cell[0][0])->readVoltage;
				readPulseWidth = static_cast<eNVM*>(arrayHO->cell[0][0])->readPulseWidth;
            }

                #pragma omp parallel for reduction(+: sumArrayReadEnergy)
				for (int j=0; j<param->nOutput; j++) {
					if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayHO->cell[0][0])) {  // Analog eNVM
						if (static_cast<eNVM*>(arrayHO->cell[0][0])->cmosAccess) {  // 1T1R
							sumArrayReadEnergy += arrayHO->wireGateCapRow * techHO.vdd * techHO.vdd * param->nHide; // All WLs open
						}
					} else if (DigitalNVM *temp = dynamic_cast<DigitalNVM*>(arrayHO->cell[0][0])) { // Digital eNVM
						if (static_cast<eNVM*>(arrayHO->cell[0][0])->cmosAccess) {  // 1T1R
							sumArrayReadEnergy += arrayHO->wireGateCapRow * techHO.vdd * techHO.vdd;    // Selected WL
						} else {    // Cross-point
							sumArrayReadEnergy += arrayHO->wireCapRow * techHO.vdd * techHO.vdd * (param->nHide - 1);   // Unselected WLs
						}
					}
                    
					for (int n=0; n<param->numBitInput; n++) {
						double pSumMaxAlgorithm = pow(2, n) / (param->numInputLevel - 1) * arrayHO->arrayRowSize;    // Max algorithm partial weighted sum for the nth vector bit (if both max input value and max weight are 1)
						if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayHO->cell[0][0])) {  // Analog eNVM
							double Isum = 0;    // weighted sum current
							double IsumMax = 0; // Max weighted sum current
              double IsumMin = 0; 
							double a1Sum = 0;    // Weighted sum current of input vector * weight=1 column                            
							for (int k=0; k<param->nHide; k++) {
								if ((da1[k]>>n) & 1) {    // if the nth bit of da1[k] is 1  
							 		Isum += arrayHO->ReadCell(j,k);
                                    a1Sum +=arrayHO->GetMediumCellReadCurrent(j,k);
                                    sumArrayReadEnergy += arrayHO->wireCapRow * readVoltage * readVoltage; // Selected BLs (1T1R) or Selected WLs (cross-point)								                                  
                                }
                                IsumMax += arrayHO->GetMaxCellReadCurrent(j,k);
                                IsumMin += arrayHO->GetMinCellReadCurrent(j,k);
							}
							sumArrayReadEnergy += Isum * readVoltage * readPulseWidth;
							int outputDigits = 2*(CurrentToDigits(Isum, IsumMax-IsumMin)-CurrentToDigits(a1Sum, IsumMax-IsumMin)); //minus the reference
							outN2[j] += DigitsToAlgorithm(outputDigits, pSumMaxAlgorithm);     
						} 
                        else 
                        {// SRAM or digital eNVM
                            bool digitalNVM = false; 
                            bool parallelRead = false;
                            if(DigitalNVM*temp = dynamic_cast<DigitalNVM*>(arrayHO->cell[0][0]))
                            {    digitalNVM = true;
                                if(static_cast<DigitalNVM*>(arrayHO->cell[0][0])->parallelRead == true) 
								{
                                    parallelRead = true;
                                }
                            }
                            if(digitalNVM && parallelRead)
                            {
                                double Imin = static_cast<DigitalNVM*>(arrayHO->cell[0][0])->avgMinConductance*static_cast<DigitalNVM*>(arrayHO->cell[0][0])->readVoltage;
                                double Imax = static_cast<DigitalNVM*>(arrayHO->cell[0][0])->avgMaxConductance*static_cast<DigitalNVM*>(arrayHO->cell[0][0])->readVoltage;
                                double Isum = 0;    // weighted sum current
                                double IsumMax = 0; // Max weighted sum current
                                double inputSum = 0;    // Weighted sum current of input vector * weight=1 column
                                int Dsum=0;
                                int DsumMax = 0;
                                int Dref = 0;
                                for (int w=0;w<param->numWeightBit;w++){
                                    int colIndex = (j+1) * param->numWeightBit - (w+1);  // w=0 is the LSB
                                    for (int k=0; k<param->nHide; k++) {
                                        if ((da1[k]>>n) & 1) { // accumulate the current along a column
                                            Isum += static_cast<DigitalNVM*>(arrayHO->cell[colIndex][k])->conductance*static_cast<DigitalNVM*>(arrayHO->cell[colIndex][k])->readVoltage;
                                            inputSum += static_cast<DigitalNVM*>(arrayHO->cell[arrayHO->refColumnNumber][k])->conductance*static_cast<DigitalNVM*>(arrayHO->cell[arrayHO->refColumnNumber][k])->readVoltage;                                            
                                        }
                                    }
                                        int outputDigits = (int) (Isum /(Imax-Imin));
                                        int outputDigitsRef = (int) (inputSum/(Imax-Imin));
                                        outputDigits = outputDigits-outputDigitsRef;
                                            
                                    Dref = (int)(inputSum/Imin);
                                    Isum=0;
                                    inputSum=0;
                                    Dsum += outputDigits*(int) pow(2,w);  // get the weight represented by the column
                                    DsumMax += param->nHide*(int) pow(2,w); // the maximum weight that can be represented by this column                                        
                                }
                                sumArrayReadEnergy += static_cast<DigitalNVM*>(arrayHO->cell[0][0])->readEnergy * arrayHO->numCellPerSynapse * arrayHO->arrayRowSize;
                                outN2[j] += (double)(Dsum - Dref*(pow(2,param->numWeightBit-1)-1)) / DsumMax * pSumMaxAlgorithm;
                            }
                            else
                            {                            
							    int Dsum = 0;
							    int DsumMax = 0;
							    int a1Sum = 0;
							    for (int k=0; k<param->nHide; k++) {
								    if ((da1[k]>>n) & 1) {    // if the nth bit of da1[k] is 1
									    Dsum += (int)(arrayHO->ReadCell(j,k));
									    a1Sum += pow(2, arrayHO->numCellPerSynapse-1) - 1;    // get current of Dummy Column as reference
								    }
								    DsumMax += pow(2, arrayHO->numCellPerSynapse) - 1;
							    } 
							    if (DigitalNVM *temp = dynamic_cast<DigitalNVM*>(arrayHO->cell[0][0])) {    // Digital eNVM
								    sumArrayReadEnergy += static_cast<DigitalNVM*>(arrayHO->cell[0][0])->readEnergy * arrayHO->numCellPerSynapse * arrayHO->arrayRowSize;
							    } 
                                else {
								    sumArrayReadEnergy += static_cast<SRAM*>(arrayHO->cell[0][0])->readEnergy * arrayHO->numCellPerSynapse * arrayHO->arrayRowSize;
							    }
							    outN2[j] += (double)(Dsum - a1Sum) / DsumMax * pSumMaxAlgorithm;
                            }
						}
					}
					a2[j] = sigmoid(outN2[j]);
				}
				arrayHO->readEnergy += sumArrayReadEnergy;
				numBatchReadSynapse = (int)ceil((double)param->nOutput/param->numColMuxed);
				// Don't parallelize this loop since there may be update of member variables inside NeuroSim functions
				for (int j=0; j<param->nOutput; j+=numBatchReadSynapse) {
					int numActiveRows = 0;  // Number of selected rows for NeuroSim
					for (int n=0; n<param->numBitInput; n++) {
						for (int k=0; k<param->nHide; k++) {
							if ((da1[k]>>n) & 1) {    // if the nth bit of da1[k] is 1
								numActiveRows++;
							}
						}
					}
					subArrayHO->activityRowRead = (double)numActiveRows/param->nHide/param->numBitInput;
					subArrayHO->readDynamicEnergy += NeuroSimSubArrayReadEnergy(subArrayHO);
					subArrayHO->readDynamicEnergy += NeuroSimNeuronReadEnergy(subArrayHO, adderHO, muxHO, muxDecoderHO, dffHO, subtractorHO);
					subArrayHO->readLatency += NeuroSimSubArrayReadLatency(subArrayHO);
					subArrayHO->readLatency += NeuroSimNeuronReadLatency(subArrayHO, adderHO, muxHO, muxDecoderHO, dffHO, subtractorHO);
				}
			} else {
				#pragma omp parallel for
				for (int j = 0; j < param->nOutput; j++) {
					for (int k = 0; k < param->nHide; k++) {
						outN2[j] += a1[k] * weight2[j][k];
					}
					a2[j] = sigmoid(outN2[j]);
				}
			}

			// Backpropagation
			/* Second layer (hidden layer to the output layer) */
			for (int j = 0; j < param->nOutput; j++){
				s2[j] = -2 * a2[j] * (1 - a2[j])*(Output[i][j] - a2[j]);
			}

			/* First layer (input layer to the hidden layer) */
			std::fill_n(s1, param->nHide, 0);
			#pragma omp parallel for
			for (int j = 0; j < param->nHide; j++) {
				for (int k = 0; k < param->nOutput; k++) {
					s1[j] += a1[j] * (1 - a1[j]) * weight2[k][j] * s2[k];
				}
			}

			// Weight update
			/* Update weight of the first layer (input layer to the hidden layer) */
			if (param->useHardwareInTrainingWU) {
				double sumArrayWriteEnergy = 0;   // Use a temporary variable here since OpenMP does not support reduction on class member
				double sumNeuroSimWriteEnergy = 0;   // Use a temporary variable here since OpenMP does not support reduction on class member
				double sumWriteLatencyAnalogNVM = 0;   // Use a temporary variable here since OpenMP does not support reduction on class member
				double numWriteOperation = 0;	// Average number of write batches in the whole array. Use a temporary variable here since OpenMP does not support reduction on class member
                double writeVoltageLTP = static_cast<eNVM*>(arrayIH->cell[0][0])->writeVoltageLTP;
                double writeVoltageLTD = static_cast<eNVM*>(arrayIH->cell[0][0])->writeVoltageLTD;
                double writePulseWidthLTP = static_cast<eNVM*>(arrayIH->cell[0][0])->writePulseWidthLTP;
                double writePulseWidthLTD = static_cast<eNVM*>(arrayIH->cell[0][0])->writePulseWidthLTD;
                if(eNVM *temp = dynamic_cast<eNVM*>(arrayIH->cell[0][0])){
                    writeVoltageLTP = static_cast<eNVM*>(arrayIH->cell[0][0])->writeVoltageLTP;
                    writeVoltageLTD = static_cast<eNVM*>(arrayIH->cell[0][0])->writeVoltageLTD;
				    writePulseWidthLTP = static_cast<eNVM*>(arrayIH->cell[0][0])->writePulseWidthLTP;
				    writePulseWidthLTD = static_cast<eNVM*>(arrayIH->cell[0][0])->writePulseWidthLTD;
                }
                numBatchWriteSynapse = (int)ceil((double)arrayIH->arrayColSize / param->numWriteColMuxed);
				#pragma omp parallel for reduction(+: sumArrayWriteEnergy, sumNeuroSimWriteEnergy, sumWriteLatencyAnalogNVM)
				for (int k = 0; k < param->nInput; k++) {
					int numWriteOperationPerRow = 0;	// Number of write batches in a row that have any weight change
					int numWriteCellPerOperation = 0;	// Average number of write cells per batch in a row (for digital eNVM)
					for (int j = 0; j < param->nHide; j+=numBatchWriteSynapse) {
						/* Batch write */
						int start = j;
						int end = j + numBatchWriteSynapse - 1;
						if (end >= param->nHide) {
							end = param->nHide - 1;
						}
						double maxLatencyLTP = 0;	// Max latency for AnalogNVM's LTP or weight increase in this batch write
						double maxLatencyLTD = 0;	// Max latency for AnalogNVM's LTD or weight decrease in this batch write
						bool weightChangeBatch = false;	// Specify if there is any weight change in the entire write batch
                        
                        double maxWeightUpdated=0;
                        double maxPulseNum =0;
                        double actualWeightUpdated;
                        for (int jj = start; jj <= end; jj++) { // Selected cells
						
							// update weight matrix
                            /*can support multiple optimization algorithm*/
                            gradt = s1[jj] * Input[i][k];
                            gradSum1[jj][k] += gradt; // sum over the gradient over all the training samples in this batch
                            if (optimization_type == "SGD"){
                                deltaWeight1[jj][k] = SGD(gradt, 1)/adLA;                        
                             }   
                            else if(optimization_type=="Momentum")
                            {
                                deltaWeight1[jj][k] = SGD(gradt, param->alpha1)/adLA; // only add momentum once every batch                       
                                if (batchSize % numTrain == 0)
                                {
                                    deltaWeight1[jj][k] = Momentum(gradt, param->alpha1,momentumPrev1[jj][k]);
                                    momentumPrev1[jj][k] = GAMA*momentumPrev1[jj][k]+param->alpha1*gradSum1[jj][k];
                                    gradSum1[jj][k] = 0;
                                }
                            }
                            else if(optimization_type=="Adagrad")
                            {
                                   deltaWeight1[jj][k] = Adagrad(gradt, param->alpha1, gradSquarePrev1[jj][k]);
                                   if (batchSize % numTrain == 0)
                                   {
                                      gradSquarePrev1[jj][k] += pow(gradSum1[jj][k],2);
                                      gradSum1[jj][k] = 0;
                                   }
                            }
                            else if(optimization_type=="RMSprop")
                            {
                                deltaWeight1[jj][k] = RMSprop(gradt, param->alpha1, gradSquarePrev1[jj][k]);
                                if (batchSize % numTrain == 0){
                                    gradSquarePrev1[jj][k] = GAMA*gradSquarePrev1[jj][k]+(1-GAMA)*pow(gradSum1[jj][k], 2);
                                    gradSum1[jj][k] = 0;
                            }
                        }
                            else if(optimization_type == "Adam")
                            {
                                deltaWeight1[jj][k] = Adam(gradt, param->alpha1, 0, gradSquarePrev1[jj][k]); //only add momentum once
                                if (batchSize % numTrain == 0){
                                deltaWeight1[jj][k] = Adam(gradt, param->alpha1, momentumPrev1[jj][k], gradSquarePrev1[jj][k]);
                                momentumPrev1[jj][k] = BETA1*momentumPrev1[jj][k]+(1-BETA1)*gradSum1[jj][k];
                                gradSquarePrev1[jj][k] = BETA2*gradSquarePrev1[jj][k]+(1-BETA2)*pow(gradSum1[jj][k], 2);
                                gradSum1[jj][k] = 0;
                            } 
                        }
                        else std::cout<<"please specify an optimization method" <<end;
                    
                           /* tracking code */
                           /*
                            totalDeltaWeight1[jj][k] += deltaWeight1[jj][k];
                            totalDeltaWeight1_abs[jj][k] += fabs(deltaWeight1[jj][k]);
                            // find the actual weight update
                            if(deltaWeight1[jj][k]+weight1[jj][k] > param-> maxWeight)
                            {
                                actualWeightUpdated=param->maxWeight - weight1[jj][k];    
                            }
                            else if(deltaWeight1[jj][k]+weight1[jj][k] < param->minWeight)
                            {
                                actualWeightUpdated=param->minWeight - weight1[jj][k];
                            } 
                            else actualWeightUpdated=deltaWeight1[jj][k];
                            
                            if(fabs(actualWeightUpdated)>maxWeightUpdated)
                            {
                                maxWeightUpdated =fabs(actualWeightUpdated);
                            }
                            */
                            
							/* weight IH update */
				
				
				    
				                           int areanumber1=0;
				                           double learningrateIH [4];
				                 /*          // classify area by index
			                                     for (int i=0; i<20; i+=k){
			                                      for (int j=i; j<400; j+=20*k){ // classify input layer area
		                                               for (int t=0; t<h; t++) {  // classify hidden layer area
							        for (int m=t*hiddenpiece; m<t*(hiddenpiece+1); m++)  {
			                                         for (int a=0; a<k; a+=1){
			                                          for (int b=0; b<20*k; b+=20){
				                                   int n = j+a+b;
							            if((jj==m) && (k==n)) {areanumber1 = 400/(20*k)*i/k+(j-i)/(20*k); break;}
								    }
								     }
								      }
							               }
							                }
							                 }  */
							             
				                               
				                           
				                           switch (areanumber1) // allocate learning rate for each area
					                    case 0:// setting learning rate for each area
				                            { learningrateIH[0] = param->learningrate[0][0];
							      learningrateIH[1] = param->learningrate[0][1];
							      learningrateIH[2] = param->learningrate[0][2];
							      learningrateIH[3] = param->learningrate[0][3];
							      break;
							    }
                            
				
							if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayIH->cell[jj][k])) {	// Analog eNVM
								
								/* new update */
								if (!stopreverse && param->ReverseUpdate){
								if ((int)(param->newUpdateRate/adNur)<(int)(param->nnewUpdateRate/adNur)){
								
								
								

								
								
								if(((batchSize+numTrain*(epochcount-1)) % (int)(param->newUpdateRate/adNur))*param->ReverseUpdate==((int)(param->newUpdateRate/adNur-1))){
									
									
								if(((batchSize+numTrain*(epochcount-1)) % (int)(param->nnewUpdateRate/adNur))*param->ReverseUpdate==((int)(param->newUpdateRate/adNur-1))){
							        arrayIH->WriteCell(jj, k, deltaWeight1[jj][k], weight1[jj][k], param->maxWeight, param->minWeight, true/*regular*/, true/*newupdate*/, false/*PositiveUpdate*/, false/*regularupdate*/, param->dominance/*dominance*/, learningrateIH);}	
								else
									
							        arrayIH->WriteCell(jj, k, deltaWeight1[jj][k], weight1[jj][k], param->maxWeight, param->minWeight, true, true, true, false, false, learningrateIH);
									
									
								}
									
								else{
								arrayIH->WriteCell(jj, k, deltaWeight1[jj][k], weight1[jj][k], param->maxWeight, param->minWeight, true, false, false, false, false, learningrateIH);
									
									
									
								}
								
									
								}
								
								else if ((int)(param->newUpdateRate/adNur)>(int)(param->nnewUpdateRate/adNur)){
									
								if(((batchSize+numTrain*(epochcount-1)) % (int)(param->nnewUpdateRate/adNur))*param->ReverseUpdate==((int)(param->nnewUpdateRate/adNur-1))){
									
									
								if(((batchSize+numTrain*(epochcount-1)) % (int)(param->newUpdateRate/adNur))*param->ReverseUpdate==((int)(param->nnewUpdateRate/adNur-1))){
							        arrayIH->WriteCell(jj, k, deltaWeight1[jj][k], weight1[jj][k], param->maxWeight, param->minWeight, true, true, true, false, param->dominance, learningrateIH);}	
								else
									
							        arrayIH->WriteCell(jj, k, deltaWeight1[jj][k], weight1[jj][k], param->maxWeight, param->minWeight, true, true, false, false, false, learningrateIH);
									
									
								}	
									
									
								else{
								arrayIH->WriteCell(jj, k, deltaWeight1[jj][k], weight1[jj][k], param->maxWeight, param->minWeight, true, false, false, false, false, learningrateIH);
									
									
									
								}	
									
									
									
									
								}
								
								else if ((int)(param->newUpdateRate/adNur)==(int)(param->nnewUpdateRate/adNur))
									
								{
								if(((batchSize+numTrain*(epochcount-1)) % (int)(param->newUpdateRate/adNur))*param->ReverseUpdate==((int)(param->newUpdateRate/adNur-1)))	
								arrayIH->WriteCell(jj, k, deltaWeight1[jj][k], weight1[jj][k], param->maxWeight, param->minWeight, true, true, false, true, false, learningrateIH);
									
								else{
								arrayIH->WriteCell(jj, k, deltaWeight1[jj][k], weight1[jj][k], param->maxWeight, param->minWeight, true, false, false, false, false, learningrateIH);
									
									
									
								}
									
								}
								
								}
							else {
							arrayIH->WriteCell(jj, k, deltaWeight1[jj][k], weight1[jj][k], param->maxWeight, param->minWeight, true, false, false, false, false, learningrateIH);
							}
									
									
							
								
								
								/**/
				
				                           /* arrayIH->WriteCell(jj, k, deltaWeight1[jj][k], weight1[jj][k], param->maxWeight, param->minWeight, true, false);*/
								
				weight1[jj][k] = arrayIH->ConductanceToWeight(jj, k, param->maxWeight, param->minWeight); 
                                weightChangeBatch = weightChangeBatch || static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->numPulse;
                                if(fabs(static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->numPulse) > maxPulseNum)
                                {
                                    maxPulseNum=fabs(static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->numPulse);
                                }
                                /* Get maxLatencyLTP and maxLatencyLTD */
								if (static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->writeLatencyLTP > maxLatencyLTP)
									maxLatencyLTP = static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->writeLatencyLTP;
								if (static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->writeLatencyLTD > maxLatencyLTD)
									maxLatencyLTD = static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->writeLatencyLTD;
							}							
       
                            else {	// SRAM and digital eNVM
                                weight1[jj][k] = weight1[jj][k] + deltaWeight1[jj][k];
								arrayIH->WriteCell(jj, k, deltaWeight1[jj][k], weight1[jj][k], param->maxWeight, param->minWeight, true);
								weightChangeBatch = weightChangeBatch || arrayIH->weightChange[jj][k];
							}
							
						}
                        // update the track variables
                        /*
                        totalWeightUpdate += maxWeightUpdated;
                        totalNumPulse += maxPulseNum; */
                        
						numWriteOperationPerRow += weightChangeBatch;
						for (int jj = start; jj <= end; jj++) { // Selected cells
							if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayIH->cell[0][0])) {  // Analog eNVM
								/* Set the max latency for all the selected cells in this batch */
								static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->writeLatencyLTP = maxLatencyLTP;
								static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->writeLatencyLTD = maxLatencyLTD;
								if (param->writeEnergyReport && weightChangeBatch) {
									if (static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->nonIdenticalPulse) {	// Non-identical write pulse scheme
										if (static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->numPulse > 0) {	// LTP
											static_cast<eNVM*>(arrayIH->cell[jj][k])->writeVoltageLTP = sqrt(static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->writeVoltageSquareSum / static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->numPulse);	// RMS value of LTP write voltage
											static_cast<eNVM*>(arrayIH->cell[jj][k])->writeVoltageLTD = static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->VinitLTD + 0.5 * static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->VstepLTD * static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->maxNumLevelLTD;	// Use average voltage of LTD write voltage
										} else if (static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->numPulse < 0) {	// LTD
											static_cast<eNVM*>(arrayIH->cell[jj][k])->writeVoltageLTP = static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->VinitLTP + 0.5 * static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->VstepLTP * static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->maxNumLevelLTP;    // Use average voltage of LTP write voltage
											static_cast<eNVM*>(arrayIH->cell[jj][k])->writeVoltageLTD = sqrt(static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->writeVoltageSquareSum / (-1*static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->numPulse));    // RMS value of LTD write voltage
										} else {	// Half-selected during LTP and LTD phases
											static_cast<eNVM*>(arrayIH->cell[jj][k])->writeVoltageLTP = static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->VinitLTP + 0.5 * static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->VstepLTP * static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->maxNumLevelLTP;    // Use average voltage of LTP write voltage
											static_cast<eNVM*>(arrayIH->cell[jj][k])->writeVoltageLTD = static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->VinitLTD + 0.5 * static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->VstepLTD * static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->maxNumLevelLTD;    // Use average voltage of LTD write voltage
										}
									}
									static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->WriteEnergyCalculation(arrayIH->wireCapCol);
									sumArrayWriteEnergy += static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->writeEnergy; 
								}
							} 
                            else if (DigitalNVM *temp = dynamic_cast<DigitalNVM*>(arrayIH->cell[0][0])) { // Digital eNVM
								if (param->writeEnergyReport && arrayIH->weightChange[jj][k]) {
									for (int n=0; n<arrayIH->numCellPerSynapse; n++) {  // n=0 is LSB
										sumArrayWriteEnergy += static_cast<DigitalNVM*>(arrayIH->cell[(jj+1) * arrayIH->numCellPerSynapse - (n+1)][k])->writeEnergy;
										int bitPrev = static_cast<DigitalNVM*>(arrayIH->cell[(jj+1) * arrayIH->numCellPerSynapse - (n+1)][k])->bitPrev;
										int bit = static_cast<DigitalNVM*>(arrayIH->cell[(jj+1) * arrayIH->numCellPerSynapse - (n+1)][k])->bit;
										if (bit != bitPrev) {
											numWriteCellPerOperation += 1;
										}
									}
								}
							} else {    // SRAM
								if (param->writeEnergyReport && arrayIH->weightChange[jj][k]) {
									sumArrayWriteEnergy += static_cast<SRAM*>(arrayIH->cell[jj * arrayIH->numCellPerSynapse][k])->writeEnergy;
								}
							}
						}
                        
						/* Latency for each batch write in Analog eNVM */
						if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayIH->cell[0][0])) {	// Analog eNVM
							sumWriteLatencyAnalogNVM += maxLatencyLTP + maxLatencyLTD;
						}
						/* Energy consumption on array caps for eNVM */
						if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayIH->cell[0][0])) {  // Analog eNVM
							if (param->writeEnergyReport && weightChangeBatch) {
								if (static_cast<AnalogNVM*>(arrayIH->cell[0][0])->nonIdenticalPulse) { // Non-identical write pulse scheme
									writeVoltageLTP = static_cast<AnalogNVM*>(arrayIH->cell[0][0])->VinitLTP + 0.5 * static_cast<AnalogNVM*>(arrayIH->cell[0][0])->VstepLTP * static_cast<AnalogNVM*>(arrayIH->cell[0][0])->maxNumLevelLTP;    // Use average voltage of LTP write voltage
									writeVoltageLTD = static_cast<AnalogNVM*>(arrayIH->cell[0][0])->VinitLTD + 0.5 * static_cast<AnalogNVM*>(arrayIH->cell[0][0])->VstepLTD * static_cast<AnalogNVM*>(arrayIH->cell[0][0])->maxNumLevelLTD;    // Use average voltage of LTD write voltage
								}
								if (static_cast<eNVM*>(arrayIH->cell[0][0])->cmosAccess) {  // 1T1R
									// The energy on selected SLs is included in WriteCell()
									sumArrayWriteEnergy += arrayIH->wireGateCapRow * techIH.vdd * techIH.vdd * 2;   // Selected WL (*2 means both LTP and LTD phases)
									sumArrayWriteEnergy += arrayIH->wireCapRow * writeVoltageLTP * writeVoltageLTP;   // Selected BL (LTP phases)
									sumArrayWriteEnergy += arrayIH->wireCapCol * writeVoltageLTP * writeVoltageLTP * (param->nHide-numBatchWriteSynapse);   // Unselected SLs (LTP phase)
									// No LTD part because all unselected rows and columns are V=0
								} else {
									sumArrayWriteEnergy += arrayIH->wireCapRow * writeVoltageLTP * writeVoltageLTP;    // Selected WL (LTP phase)
									sumArrayWriteEnergy += arrayIH->wireCapRow * writeVoltageLTP/2 * writeVoltageLTP/2 * (param->nInput - 1);  // Unselected WLs (LTP phase)
									sumArrayWriteEnergy += arrayIH->wireCapCol * writeVoltageLTP/2 * writeVoltageLTP/2 * (param->nHide - numBatchWriteSynapse);   // Unselected BLs (LTP phase)
									sumArrayWriteEnergy += arrayIH->wireCapRow * writeVoltageLTD/2 * writeVoltageLTD/2 * (param->nInput - 1);    // Unselected WLs (LTD phase)
									sumArrayWriteEnergy += arrayIH->wireCapCol * writeVoltageLTD/2 * writeVoltageLTD/2 * (param->nHide - numBatchWriteSynapse); // Unselected BLs (LTD phase)
								}
							}
						}
                        else if (DigitalNVM *temp = dynamic_cast<DigitalNVM*>(arrayIH->cell[0][0])) { // Digital eNVM
							if (param->writeEnergyReport && weightChangeBatch) {
								if (static_cast<eNVM*>(arrayIH->cell[0][0])->cmosAccess) {  // 1T1R
									// The energy on selected columns is included in WriteCell()
									sumArrayWriteEnergy += arrayIH->wireGateCapRow * techIH.vdd * techIH.vdd * 2;   // Selected WL (*2 for both SET and RESET phases)
								} else {    // Cross-point
									sumArrayWriteEnergy += arrayIH->wireCapRow * writeVoltageLTP * writeVoltageLTP;   // Selected WL (SET phase)
									sumArrayWriteEnergy += arrayIH->wireCapRow * writeVoltageLTP/2 * writeVoltageLTP/2 * (param->nInput - 1);    // Unselected WLs (SET phase)
									sumArrayWriteEnergy += arrayIH->wireCapCol * writeVoltageLTP/2 * writeVoltageLTP/2 * (param->nHide - numBatchWriteSynapse) * arrayIH->numCellPerSynapse;   // Unselected BLs (SET phase)
									sumArrayWriteEnergy += arrayIH->wireCapRow * writeVoltageLTD/2 * writeVoltageLTD/2 * (param->nInput - 1);   // Unselected WLs (RESET phase)
									sumArrayWriteEnergy += arrayIH->wireCapCol * writeVoltageLTD/2 * writeVoltageLTD/2 * (param->nHide - numBatchWriteSynapse) * arrayIH->numCellPerSynapse;   // Unselected BLs (RESET phase)
								}
							}
						}
						/* Half-selected cells for eNVM */
						if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayIH->cell[0][0])) {  // Analog eNVM
							if (!static_cast<eNVM*>(arrayIH->cell[0][0])->cmosAccess && param->writeEnergyReport) { // Cross-point
								for (int jj = 0; jj < param->nHide; jj++) { // Half-selected cells in the same row
									if (jj >= start && jj <= end) { continue; } // Skip the selected cells
									sumArrayWriteEnergy += (writeVoltageLTP/2 * writeVoltageLTP/2 * static_cast<eNVM*>(arrayIH->cell[jj][k])->conductanceAtHalfVwLTP * maxLatencyLTP + writeVoltageLTD/2 * writeVoltageLTD/2 * static_cast<eNVM*>(arrayIH->cell[jj][k])->conductanceAtHalfVwLTD * maxLatencyLTD);
								}
								for (int kk = 0; kk < param->nInput; kk++) {    // Half-selected cells in other rows
									// Note that here is a bit inaccurate if using OpenMP, because the weight on other rows (threads) are also being updated
									if (kk == k) { continue; } // Skip the selected row
									for (int jj = start; jj <= end; jj++) {
										sumArrayWriteEnergy += (writeVoltageLTP/2 * writeVoltageLTP/2 * static_cast<eNVM*>(arrayIH->cell[jj][kk])->conductanceAtHalfVwLTP * maxLatencyLTP + writeVoltageLTD/2 * writeVoltageLTD/2 * static_cast<eNVM*>(arrayIH->cell[jj][kk])->conductanceAtHalfVwLTD * maxLatencyLTD);
									}
								}
							}
						} else if (DigitalNVM *temp = dynamic_cast<DigitalNVM*>(arrayIH->cell[0][0])) { // Digital eNVM
							if (!static_cast<eNVM*>(arrayIH->cell[0][0])->cmosAccess && param->writeEnergyReport && weightChangeBatch) { // Cross-point
								for (int jj = 0; jj < param->nHide; jj++) {    // Half-selected synapses in the same row
									if (jj >= start && jj <= end) { continue; } // Skip the selected synapses
									for (int n=0; n<arrayIH->numCellPerSynapse; n++) {  // n=0 is LSB
										int colIndex = (jj+1) * arrayIH->numCellPerSynapse - (n+1);
										sumArrayWriteEnergy += writeVoltageLTP/2 * writeVoltageLTP/2 * static_cast<eNVM*>(arrayIH->cell[colIndex][k])->conductanceAtHalfVwLTP * maxLatencyLTP + writeVoltageLTD/2 * writeVoltageLTD/2 * static_cast<eNVM*>(arrayIH->cell[colIndex][k])->conductanceAtHalfVwLTD * maxLatencyLTD;
									}
								}
								for (int kk = 0; kk < param->nInput; kk++) {   // Half-selected synapses in other rows
									// Note that here is a bit inaccurate if using OpenMP, because the weight on other rows (threads) are also being updated
									if (kk == k) { continue; } // Skip the selected row
									for (int jj = start; jj <= end; jj++) {
										for (int n=0; n<arrayIH->numCellPerSynapse; n++) {  // n=0 is LSB
											int colIndex = (jj+1) * arrayIH->numCellPerSynapse - (n+1);
											sumArrayWriteEnergy += writeVoltageLTP/2 * writeVoltageLTP/2 * static_cast<eNVM*>(arrayIH->cell[colIndex][kk])->conductanceAtHalfVwLTP * maxLatencyLTP + writeVoltageLTD/2 * writeVoltageLTD/2 * static_cast<eNVM*>(arrayIH->cell[colIndex][kk])->conductanceAtHalfVwLTD * maxLatencyLTD;
										}
									}
								}
							}
						}
					}
					/* Calculate the average number of write pulses on the selected row */
					#pragma omp critical    // Use critical here since NeuroSim class functions may update its member variables
					{
						if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayIH->cell[0][0])) {  // Analog eNVM
							int sumNumWritePulse = 0;
							for (int j = 0; j < param->nHide; j++) {
								sumNumWritePulse += abs(static_cast<AnalogNVM*>(arrayIH->cell[j][k])->numPulse);    // Note that LTD has negative pulse number
							}
							subArrayIH->numWritePulse = sumNumWritePulse / param->nHide;
							double writeVoltageSquareSumRow = 0;
							if (param->writeEnergyReport) {
								if (static_cast<AnalogNVM*>(arrayIH->cell[0][0])->nonIdenticalPulse) { // Non-identical write pulse scheme
									for (int j = 0; j < param->nHide; j++) {
										writeVoltageSquareSumRow += static_cast<AnalogNVM*>(arrayIH->cell[j][k])->writeVoltageSquareSum;
									}
									if (sumNumWritePulse > 0) {	// Prevent division by 0
										subArrayIH->cell.writeVoltage = sqrt(writeVoltageSquareSumRow / sumNumWritePulse);	// RMS value of write voltage in a row
									} else {
										subArrayIH->cell.writeVoltage = 0;
									}
								}
							}
						}
						numWriteCellPerOperation = (double)numWriteCellPerOperation/numWriteOperationPerRow;
						sumNeuroSimWriteEnergy += NeuroSimSubArrayWriteEnergy(subArrayIH, numWriteOperationPerRow, numWriteCellPerOperation);
					}
					numWriteOperation += numWriteOperationPerRow;
                    sumNeuroSimWriteEnergy += NeuroSimSubArrayWriteEnergy(subArrayIH, numWriteOperationPerRow, numWriteCellPerOperation);
				}
				arrayIH->writeEnergy += sumArrayWriteEnergy;
				subArrayIH->writeDynamicEnergy += sumNeuroSimWriteEnergy;
				numWriteOperation = numWriteOperation / param->nInput;
				subArrayIH->writeLatency += NeuroSimSubArrayWriteLatency(subArrayIH, numWriteOperation, sumWriteLatencyAnalogNVM);
			} else {
				#pragma omp parallel for
				for (int j = 0; j < param->nHide; j++) {
					for (int k = 0; k < param->nInput; k++) {
						deltaWeight1[j][k] = - param->alpha1 * s1[j] * Input[i][k];
						weight1[j][k] = weight1[j][k] + deltaWeight1[j][k];
						if (weight1[j][k] > param->maxWeight) {
							deltaWeight1[j][k] -= weight1[j][k] - param->maxWeight;
							weight1[j][k] = param->maxWeight;
						} else if (weight1[j][k] < param->minWeight) {
							deltaWeight1[j][k] += param->minWeight - weight1[j][k];
							weight1[j][k] = param->minWeight;
						}
						if (param->useHardwareInTrainingFF) {
							arrayIH->WriteCell(j, k, deltaWeight1[j][k], weight1[j][k], param->maxWeight, param->minWeight, false);
						}
					}
				}
			}

			/* Update weight of the second layer (hidden layer to the output layer) */
			if (param->useHardwareInTrainingWU) {
				double sumArrayWriteEnergy = 0;   // Use a temporary variable here since OpenMP does not support reduction on class member
				double sumNeuroSimWriteEnergy = 0;   // Use a temporary variable here since OpenMP does not support reduction on class member
				double sumWriteLatencyAnalogNVM = 0;	// Use a temporary variable here since OpenMP does not support reduction on class member
				double numWriteOperation = 0;	// Average number of write batches in the whole array. Use a temporary variable here since OpenMP does not support reduction on class member
                double writeVoltageLTP;
                double writeVoltageLTD;
                double writePulseWidthLTP;
                double writePulseWidthLTD;				
                if(eNVM *temp = dynamic_cast<eNVM*>(arrayHO->cell[0][0])){
                     writeVoltageLTP = static_cast<eNVM*>(arrayHO->cell[0][0])->writeVoltageLTP;
				     writeVoltageLTD = static_cast<eNVM*>(arrayHO->cell[0][0])->writeVoltageLTD;
				     writePulseWidthLTP = static_cast<eNVM*>(arrayHO->cell[0][0])->writePulseWidthLTP;
				     writePulseWidthLTD = static_cast<eNVM*>(arrayHO->cell[0][0])->writePulseWidthLTD;
                }
				numBatchWriteSynapse = (int)ceil((double)arrayHO->arrayColSize / param->numWriteColMuxed);
				#pragma omp parallel for reduction(+: sumArrayWriteEnergy, sumNeuroSimWriteEnergy, sumWriteLatencyAnalogNVM)
				for (int k = 0; k < param->nHide; k++) {
					int numWriteOperationPerRow = 0;    // Number of write batches in a row that have any weight change
					int numWriteCellPerOperation = 0;   // Average number of write cells per batch in a row (for digital eNVM)
					for (int j = 0; j < param->nOutput; j+=numBatchWriteSynapse) {
						/* Batch write */
						int start = j;
						int end = j + numBatchWriteSynapse - 1;
						if (end >= param->nOutput) {
							end = param->nOutput - 1;
						}
						double maxLatencyLTP = 0;   // Max latency for AnalogNVM's LTP or weight increase in this batch write
						double maxLatencyLTD = 0;   // Max latency for AnalogNVM's LTD or weight decrease in this batch write
						bool weightChangeBatch = false; // Specify if there is any weight change in the entire write batch
                        
                        double maxWeightUpdated=0;
                        double maxPulseNum =0;
                        double actualWeightUpdated=0;
                        for (int jj = start; jj <= end; jj++) { // Selected cells

                            gradt = s2[jj] * a1[k];
                            gradSum2[jj][k] += gradt; // sum over the gradient over all the training samples in this batch
                         if (optimization_type == "SGD") 
                            deltaWeight2[jj][k] = SGD(gradt, 1/param->speed)/adLA;                        
                            else if(optimization_type=="Momentum")
                                    {
                                        deltaWeight2[jj][k] = SGD(gradt, param->alpha2)/adLA;                        
                                        if (batchSize % numTrain == 0){                                            
                                            deltaWeight2[jj][k] = Momentum(gradt, param->alpha2,momentumPrev2[jj][k]);
                                            momentumPrev2[jj][k] = GAMA*momentumPrev2[jj][k]+param->alpha2*gradSum2[jj][k];
                                            gradSum2[jj][k] = 0;
                                        }
                                    }
                            else if(optimization_type=="Adagrad")
                                    {
                                        deltaWeight2[jj][k] = Adagrad(gradt, param->alpha2, gradSquarePrev2[jj][k]);
                                        if (batchSize % numTrain == 0){
                                            gradSquarePrev2[jj][k] += pow(gradSum2[jj][k],2);
                                            gradSum2[jj][k] = 0;
                                        }
                                    }
                            else if(optimization_type=="RMSprop")
                                    {
                                        deltaWeight2[jj][k] = RMSprop(gradt, param->alpha2, gradSquarePrev2[jj][k]);
                                        if (batchSize % numTrain == 0){
                                            gradSquarePrev2[jj][k] = GAMA*gradSquarePrev2[jj][k]+(1-GAMA)*pow(gradSum2[jj][k], 2);
                                            gradSum2[jj][k] = 0;
                                        }
                                    }
                            else if(optimization_type == "Adam")
                                   {
                                        deltaWeight2[jj][k] = Adam(gradt, param->alpha2, 0, gradSquarePrev2[jj][k]);

                                        if (batchSize % numTrain == 0){
                                            deltaWeight2[jj][k] = Adam(gradt, param->alpha2, momentumPrev2[jj][k], gradSquarePrev2[jj][k]);
                                            momentumPrev2[jj][k] = BETA1*momentumPrev2[jj][k]+(1-BETA1)*gradSum2[jj][k];
                                            gradSquarePrev2[jj][k] = BETA2*gradSquarePrev2[jj][k]+(1-BETA2)*pow(gradSum2[jj][k], 2);
                                            gradSum2[jj][k] = 0;
                                        } 
                                  }
                            else std::cout<<"please specify an optimization method" <<end;

                            // the gradSquarePrev and mementumPrev are updated inside the function
                            /*tracking code*/
                            /*
                            totalDeltaWeight2[jj][k] += deltaWeight2[jj][k];
                            totalDeltaWeight2_abs[jj][k] += fabs(deltaWeight2[jj][k]);
                            */
                          
                            /* track the number of weight update*/
                            /*// find the actual weight update
                            if(deltaWeight2[jj][k]+weight2[jj][k] > param-> maxWeight)
                            {
                                actualWeightUpdated=param->maxWeight - weight2[jj][k];    
                            }
                            else if(deltaWeight2[jj][k]+weight2[jj][k] < param->minWeight)
                            {
                                actualWeightUpdated=param->minWeight - weight2[jj][k];
                            } 
                            else actualWeightUpdated=deltaWeight2[jj][k]; 
                            
                            //if( fabs(deltaWeight1[jj][k]) > maxWeightUpdated )
                            if(fabs(actualWeightUpdated)>maxWeightUpdated)
                            {
                                maxWeightUpdated =fabs(actualWeightUpdated);
                                // maxWeightUpdated =fabs(deltaWeight2[jj][k]);
                            } */
                            
                         /*   if( fabs(deltaWeight2[jj][k]) > maxWeightUpdated )
                            {
                                maxWeightUpdated =fabs(deltaWeight2[jj][k]);
                            }
                        */			        /* weight HO update */
				                           int areanumber2=0;
				                           double learningrateHO [4];
				                           // classify area by index
                                                          
								   
		/*	for (int m=0; m<param->nOutput; m++) { // classify output area
			 for (int t=0; t<h; t++) {  // classify hidden layer area
			   for (int n=t*hiddenpiece; n<t*(hiddenpiece+1); n++)  {
			       if((jj==m) && (k==n)) {areanumber2 = 400/(20*k)*(20/k)+h*m+t; break;}
			      }
			       }
			        }    */
								     
				                               
				                           
				                           switch (areanumber2) // allocate learning rate for each area
					                    case 0:// setting learning rate for each area
				                            { learningrateHO[0] = param->learningrate[1][0];
							      learningrateHO[1] = param->learningrate[1][1];
							      learningrateHO[2] = param->learningrate[1][2];
							      learningrateHO[3] = param->learningrate[1][3];
							      break;
							    }
				
							if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayHO->cell[jj][k])) { // Analog eNVM
								
								/* new update */
								if (!stopreverse && param->ReverseUpdate){
								if ((int)(param->newUpdateRate/adNur)<(int)(param->nnewUpdateRate/adNur)){
								
								
								

								
								
								if(((batchSize+numTrain*(epochcount-1)) % (int)(param->newUpdateRate/adNur))*param->ReverseUpdate==((int)(param->newUpdateRate/adNur-1))){
									
									
								if(((batchSize+numTrain*(epochcount-1)) % (int)(param->nnewUpdateRate/adNur))*param->ReverseUpdate==((int)(param->newUpdateRate/adNur-1))){
							        arrayHO->WriteCell(jj, k, deltaWeight2[jj][k], weight2[jj][k], param->maxWeight, param->minWeight, true/*regular*/, true/*newupdate*/, false/*PositiveUpdate*/, false/*regularupdate*/, param->dominance/*dominance*/, learningrateHO);}	
								else
									
							        arrayHO->WriteCell(jj, k, deltaWeight2[jj][k], weight2[jj][k], param->maxWeight, param->minWeight, true, true, true, false, false, learningrateHO);
									
									
								}
									
								else{
								arrayHO->WriteCell(jj, k, deltaWeight2[jj][k], weight2[jj][k], param->maxWeight, param->minWeight, true, false, false, false, false, learningrateHO);
									
									
									
								}
								
									
								}
								
								else if ((int)(param->newUpdateRate/adNur)>(int)(param->nnewUpdateRate/adNur)){
									
								if(((batchSize+numTrain*(epochcount-1)) % (int)(param->nnewUpdateRate/adNur))*param->ReverseUpdate==(int)(param->nnewUpdateRate/adNur-1)){
									
									
								if(((batchSize+numTrain*(epochcount-1)) % (int)(param->newUpdateRate/adNur))*param->ReverseUpdate==(int)(param->nnewUpdateRate/adNur-1)){
							        arrayHO->WriteCell(jj, k, deltaWeight2[jj][k], weight2[jj][k], param->maxWeight, param->minWeight, true, true, true, false, param->dominance, learningrateHO);}	
								else
									
							        arrayHO->WriteCell(jj, k, deltaWeight2[jj][k], weight2[jj][k], param->maxWeight, param->minWeight, true, true, false, false, false, learningrateHO);
									
									
								}	
									
									
								else{
								arrayHO->WriteCell(jj, k, deltaWeight2[jj][k], weight2[jj][k], param->maxWeight, param->minWeight, true, false, false, false, false, learningrateHO);
									
									
									
								}	
									
									
									
									
								}
								
								else if ((int)(param->newUpdateRate/adNur)==(int)(param->nnewUpdateRate/adNur))
									
								{
								if(((batchSize+numTrain*(epochcount-1)) % (int)(param->newUpdateRate/adNur))*param->ReverseUpdate==((int)(param->newUpdateRate/adNur-1)))	
								arrayHO->WriteCell(jj, k, deltaWeight2[jj][k], weight2[jj][k], param->maxWeight, param->minWeight, true, true, false, true, false, learningrateHO);
									
								else{
								arrayHO->WriteCell(jj, k, deltaWeight2[jj][k], weight2[jj][k], param->maxWeight, param->minWeight, true, false, false, false, false, learningrateHO);
									
									
									
								}
									
								}
								
								}
							
								else{
								arrayHO->WriteCell(jj, k, deltaWeight2[jj][k], weight2[jj][k], param->maxWeight, param->minWeight, true, false, false, false, false, learningrateHO);
								}
								
								
								
								
							
								
								/**/
								/* arrayHO->WriteCell(jj, k, deltaWeight2[jj][k], weight2[jj][k], param->maxWeight, param->minWeight, true, false); */
				
						                weight2[jj][k] = arrayHO->ConductanceToWeight(jj, k, param->maxWeight, param->minWeight);
								weightChangeBatch = weightChangeBatch || static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->numPulse;
								
                                {
                                    maxPulseNum=fabs(static_cast<AnalogNVM*>(arrayIH->cell[jj][k])->numPulse);
                                }
                                /* Get maxLatencyLTP and maxLatencyLTD */
								if (static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->writeLatencyLTP > maxLatencyLTP)
									maxLatencyLTP = static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->writeLatencyLTP;
								if (static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->writeLatencyLTD > maxLatencyLTD)
									maxLatencyLTD = static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->writeLatencyLTD;
							}
                            else {    // SRAM and digital eNVM
								weight2[jj][k] = weight2[jj][k] + deltaWeight2[jj][k];
								arrayHO->WriteCell(jj, k, deltaWeight2[jj][k], weight2[jj][k], param->maxWeight, param->minWeight, true);
								weightChangeBatch = weightChangeBatch || arrayHO->weightChange[jj][k];
							}
							
						}
                        totalWeightUpdate += maxWeightUpdated;
                        totalNumPulse += maxPulseNum;
                        
                        /* Latency for each batch write in Analog eNVM */
						numWriteOperationPerRow += weightChangeBatch;
						for (int jj = start; jj <= end; jj++) { // Selected cells
							if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayHO->cell[0][0])) {  // Analog eNVM
								/* Set the max latency for all the cells in this batch */
								static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->writeLatencyLTP = maxLatencyLTP;
								static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->writeLatencyLTD = maxLatencyLTD;
								if (param->writeEnergyReport && weightChangeBatch) {
									if (static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->nonIdenticalPulse) { // Non-identical write pulse scheme
										if (static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->numPulse > 0) {  // LTP
											static_cast<eNVM*>(arrayHO->cell[jj][k])->writeVoltageLTP = sqrt(static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->writeVoltageSquareSum / static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->numPulse);   // RMS value of LTP write voltage
											static_cast<eNVM*>(arrayHO->cell[jj][k])->writeVoltageLTD = static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->VinitLTD + 0.5 * static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->VstepLTD * static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->maxNumLevelLTD;    // Use average voltage of LTD write voltage
										} else if (static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->numPulse < 0) {    // LTD
											static_cast<eNVM*>(arrayHO->cell[jj][k])->writeVoltageLTP = static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->VinitLTP + 0.5 * static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->VstepLTP * static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->maxNumLevelLTP;    // Use average voltage of LTP write voltage
											static_cast<eNVM*>(arrayHO->cell[jj][k])->writeVoltageLTD = sqrt(static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->writeVoltageSquareSum / (-1*static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->numPulse));    // RMS value of LTD write voltage
										} else {	// Half-selected during LTP and LTD phases
											static_cast<eNVM*>(arrayHO->cell[jj][k])->writeVoltageLTP = static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->VinitLTP + 0.5 * static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->VstepLTP * static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->maxNumLevelLTP;    // Use average voltage of LTP write voltage
											static_cast<eNVM*>(arrayHO->cell[jj][k])->writeVoltageLTD = static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->VinitLTD + 0.5 * static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->VstepLTD * static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->maxNumLevelLTD;    // Use average voltage of LTD write voltage
										}
									}
									static_cast<AnalogNVM*>(arrayHO->cell[jj][k])->WriteEnergyCalculation(arrayHO->wireCapCol);
									sumArrayWriteEnergy += static_cast<eNVM*>(arrayHO->cell[jj][k])->writeEnergy;
								}
							}
                            else if (DigitalNVM *temp = dynamic_cast<DigitalNVM*>(arrayHO->cell[0][0])) { // Digital eNVM
								if (param->writeEnergyReport && arrayHO->weightChange[jj][k]) {
									for (int n=0; n<arrayHO->numCellPerSynapse; n++) {  // n=0 is LSB
										sumArrayWriteEnergy += static_cast<DigitalNVM*>(arrayHO->cell[(jj+1) * arrayHO->numCellPerSynapse - (n+1)][k])->writeEnergy;
										int bitPrev = static_cast<DigitalNVM*>(arrayHO->cell[(jj+1) * arrayHO->numCellPerSynapse - (n+1)][k])->bitPrev;
										int bit = static_cast<DigitalNVM*>(arrayHO->cell[(jj+1) * arrayHO->numCellPerSynapse - (n+1)][k])->bit;
										if (bit != bitPrev) {
											numWriteCellPerOperation += 1;
										}
									}
								}
							} else {    // SRAM
								if (param->writeEnergyReport && arrayHO->weightChange[jj][k]) {
									sumArrayWriteEnergy += static_cast<SRAM*>(arrayHO->cell[jj * arrayHO->numCellPerSynapse][k])->writeEnergy;
								}
							}
						}
						/* Latency for each batch write in Analog eNVM */
						if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayHO->cell[0][0])) {  // Analog eNVM
							sumWriteLatencyAnalogNVM += maxLatencyLTP + maxLatencyLTD;
						}
						/* Energy consumption on array caps for eNVM */
						if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayHO->cell[0][0])) {  // Analog eNVM
							if (param->writeEnergyReport && weightChangeBatch) {
								if (static_cast<AnalogNVM*>(arrayHO->cell[0][0])->nonIdenticalPulse) { // Non-identical write pulse scheme
									writeVoltageLTP = static_cast<AnalogNVM*>(arrayHO->cell[0][0])->VinitLTP + 0.5 * static_cast<AnalogNVM*>(arrayHO->cell[0][0])->VstepLTP * static_cast<AnalogNVM*>(arrayHO->cell[0][0])->maxNumLevelLTP;    // Use average voltage of LTP write voltage
									writeVoltageLTD = static_cast<AnalogNVM*>(arrayHO->cell[0][0])->VinitLTD + 0.5 * static_cast<AnalogNVM*>(arrayHO->cell[0][0])->VstepLTD * static_cast<AnalogNVM*>(arrayHO->cell[0][0])->maxNumLevelLTD;    // Use average voltage of LTD write voltage
								}
								if (static_cast<eNVM*>(arrayHO->cell[0][0])->cmosAccess) {  // 1T1R
									// The energy on selected SLs is included in WriteCell()
									sumArrayWriteEnergy += arrayHO->wireGateCapRow * techHO.vdd * techHO.vdd * 2;   // Selected WL (*2 means both LTP and LTD phases)
									sumArrayWriteEnergy += arrayHO->wireCapRow * writeVoltageLTP * writeVoltageLTP;   // Selected BL (LTP phases)
									sumArrayWriteEnergy += arrayHO->wireCapCol * writeVoltageLTP * writeVoltageLTP * (param->nOutput-numBatchWriteSynapse);   // Unselected SLs (LTP phase)
									// No LTD part because all unselected rows and columns are V=0
								} else {
									sumArrayWriteEnergy += arrayHO->wireCapRow * writeVoltageLTP * writeVoltageLTP;   // Selected WL (LTP phase)
									sumArrayWriteEnergy += arrayHO->wireCapRow * writeVoltageLTP/2 * writeVoltageLTP/2 * (param->nHide - 1);    // Unselected WLs (LTP phase)
									sumArrayWriteEnergy += arrayHO->wireCapCol * writeVoltageLTP/2 * writeVoltageLTP/2 * (param->nOutput - numBatchWriteSynapse); // Unselected BLs (LTP phase)
									sumArrayWriteEnergy += arrayHO->wireCapRow * writeVoltageLTD/2 * writeVoltageLTD/2 * (param->nHide - 1);    // Unselected WLs (LTD phase)
									sumArrayWriteEnergy += arrayHO->wireCapCol * writeVoltageLTD/2 * writeVoltageLTD/2 * (param->nOutput - numBatchWriteSynapse); // Unselected BLs (LTD phase)
								}
							}
						}
                        else if (DigitalNVM *temp = dynamic_cast<DigitalNVM*>(arrayHO->cell[0][0])) { // Digital eNVM
							if (param->writeEnergyReport && weightChangeBatch) {
								if (static_cast<eNVM*>(arrayHO->cell[0][0])->cmosAccess) {  // 1T1R
									// The energy on selected columns is included in WriteCell()
									sumArrayWriteEnergy += arrayHO->wireGateCapRow * techHO.vdd * techHO.vdd * 2;   // Selected WL (*2 for both SET and RESET phases)
								} else {    // Cross-point
									sumArrayWriteEnergy += arrayHO->wireCapRow * writeVoltageLTP * writeVoltageLTP;   // Selected WL (SET phase)
									sumArrayWriteEnergy += arrayHO->wireCapRow * writeVoltageLTP/2 * writeVoltageLTP/2 * (param->nInput - 1);    // Unselected WLs (SET phase)
									sumArrayWriteEnergy += arrayHO->wireCapCol * writeVoltageLTP/2 * writeVoltageLTP/2 * (param->nHide - numBatchWriteSynapse) * arrayHO->numCellPerSynapse;  // Unselected BLs (SET phase)
									sumArrayWriteEnergy += arrayHO->wireCapRow * writeVoltageLTD/2 * writeVoltageLTD/2 * (param->nInput - 1);  // Unselected WLs (RESET phase)
									sumArrayWriteEnergy += arrayHO->wireCapCol * writeVoltageLTD/2 * writeVoltageLTD/2 * (param->nHide - numBatchWriteSynapse) * arrayHO->numCellPerSynapse;  // Unselected BLs (RESET phase)
								}
							}
						}
						/* Half-selected cells for eNVM */
						if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayHO->cell[0][0])) {  // Analog eNVM
							if (!static_cast<eNVM*>(arrayHO->cell[0][0])->cmosAccess && param->writeEnergyReport) { // Cross-point
								for (int jj = 0; jj < param->nOutput; jj++) {    // Half-selected cells in the same row
									if (jj >= start && jj <= end) { continue; } // Skip the selected cells
									sumArrayWriteEnergy += (writeVoltageLTP/2 * writeVoltageLTP/2 * static_cast<eNVM*>(arrayHO->cell[jj][k])->conductanceAtHalfVwLTP * maxLatencyLTP + writeVoltageLTD/2 * writeVoltageLTD/2 * static_cast<eNVM*>(arrayHO->cell[jj][k])->conductanceAtHalfVwLTD * maxLatencyLTD);
								}
								for (int kk = 0; kk < param->nHide; kk++) { // Half-selected cells in other rows
									// Note that here is a bit inaccurate if using OpenMP, because the weight on other rows (threads) are also being updated
									if (kk == k) { continue; }  // Skip the selected row
									for (int jj = start; jj <= end; jj++) {
										sumArrayWriteEnergy += (writeVoltageLTP/2 * writeVoltageLTP/2 * static_cast<eNVM*>(arrayHO->cell[jj][kk])->conductanceAtHalfVwLTP * maxLatencyLTP + writeVoltageLTD/2 * writeVoltageLTD/2 * static_cast<eNVM*>(arrayHO->cell[jj][kk])->conductanceAtHalfVwLTD * maxLatencyLTD);
									}
								}
							}
						} else if (DigitalNVM *temp = dynamic_cast<DigitalNVM*>(arrayHO->cell[0][0])) { // Digital eNVM
							if (!static_cast<eNVM*>(arrayHO->cell[0][0])->cmosAccess && param->writeEnergyReport && weightChangeBatch) { // Cross-point
								for (int jj = 0; jj < param->nOutput; jj++) {    // Half-selected synapses in the same row
									if (jj >= start && jj <= end) { continue; } // Skip the selected synapses
									for (int n=0; n<arrayHO->numCellPerSynapse; n++) {  // n=0 is LSB
										int colIndex = (jj+1) * arrayHO->numCellPerSynapse - (n+1);
										sumArrayWriteEnergy += writeVoltageLTP/2 * writeVoltageLTP/2 * static_cast<eNVM*>(arrayHO->cell[colIndex][k])->conductanceAtHalfVwLTP * maxLatencyLTP + writeVoltageLTD/2 * writeVoltageLTD/2 * static_cast<eNVM*>(arrayHO->cell[colIndex][k])->conductanceAtHalfVwLTD * maxLatencyLTD;
									}
								}
								for (int kk = 0; kk < param->nHide; kk++) {    // Half-selected synapses in other rows
									// Note that here is a bit inaccurate if using OpenMP, because the weight on other rows (threads) are also being updated
									if (kk == k) { continue; }  // Skip the selected row
									for (int jj = start; jj <= end; jj++) {
										for (int n=0; n<arrayHO->numCellPerSynapse; n++) {  // n=0 is LSB
											int colIndex = (jj+1) * arrayHO->numCellPerSynapse - (n+1);
											sumArrayWriteEnergy += writeVoltageLTP/2 * writeVoltageLTP/2 * static_cast<eNVM*>(arrayHO->cell[colIndex][kk])->conductanceAtHalfVwLTP * maxLatencyLTP + writeVoltageLTD/2 * writeVoltageLTD/2 * static_cast<eNVM*>(arrayHO->cell[colIndex][kk])->conductanceAtHalfVwLTD * maxLatencyLTD;
										}
									}
								}
							}
						}
					}
					/* Calculate the average number of write pulses on the selected row */
					#pragma omp critical    // Use critical here since NeuroSim class functions may update its member variables
					{
						if (AnalogNVM *temp = dynamic_cast<AnalogNVM*>(arrayHO->cell[0][0])) {  // Analog eNVM
							int sumNumWritePulse = 0;
							for (int j = 0; j < param->nOutput; j++) {
								sumNumWritePulse += abs(static_cast<AnalogNVM*>(arrayHO->cell[j][k])->numPulse);    // Note that LTD has negative pulse number
							}
							subArrayHO->numWritePulse = sumNumWritePulse / param->nOutput;
							double writeVoltageSquareSumRow = 0;
							if (param->writeEnergyReport) {
								if (static_cast<AnalogNVM*>(arrayHO->cell[0][0])->nonIdenticalPulse) { // Non-identical write pulse scheme
									for (int j = 0; j < param->nOutput; j++) {
										writeVoltageSquareSumRow += static_cast<AnalogNVM*>(arrayHO->cell[j][k])->writeVoltageSquareSum;
									}
									if (sumNumWritePulse > 0) {	// Prevent division by 0
										subArrayHO->cell.writeVoltage = sqrt(writeVoltageSquareSumRow / sumNumWritePulse);  // RMS value of write voltage in a row
									} else {
										subArrayHO->cell.writeVoltage = 0;
									}
								}
							}
						}
						numWriteCellPerOperation = (double)numWriteCellPerOperation/numWriteOperationPerRow;
						sumNeuroSimWriteEnergy += NeuroSimSubArrayWriteEnergy(subArrayHO, numWriteOperationPerRow, numWriteCellPerOperation);
					}
					numWriteOperation += numWriteOperationPerRow;
				}
				arrayHO->writeEnergy += sumArrayWriteEnergy;
				subArrayHO->writeDynamicEnergy += sumNeuroSimWriteEnergy;
				numWriteOperation = numWriteOperation / param->nHide;
				subArrayHO->writeLatency += NeuroSimSubArrayWriteLatency(subArrayHO, numWriteOperation, sumWriteLatencyAnalogNVM);
			} else {
				#pragma omp parallel for
				for (int j = 0; j < param->nOutput; j++) {
					for (int k = 0; k < param->nHide; k++) {
						deltaWeight2[j][k] = -param->alpha2 * s2[j] * a1[k];
						weight2[j][k] = weight2[j][k] + deltaWeight2[j][k];
						if (weight2[j][k] > param->maxWeight) {
							deltaWeight2[j][k] -= weight2[j][k] - param->maxWeight;
							weight2[j][k] = param->maxWeight;
						} else if (weight2[j][k] < param->minWeight) {
							deltaWeight2[j][k] += param->minWeight - weight2[j][k];
							weight2[j][k] = param->minWeight;
						}
						if (param->useHardwareInTrainingFF) {
							arrayHO->WriteCell(j, k, deltaWeight2[j][k], weight2[j][k], param->maxWeight, param->minWeight, false);
						}
					}
				}
			}
			
	      /* conductance saturation management: Full-Reset */ 
			if(!stopreset&&(int)(param -> FullRefresh)/adFrr){
				
			if ((batchSize+numTrain*(epochcount-1)) % param->RefreshRate == (param->RefreshRate-1)) { //ERASE
				for (int j = 0; j < param->nHide; j++) {
					for (int k = 0; k < param->nInput; k++) {
						arrayIH->EraseCell(j,k);
						//std::cout << arrayIH->ConductanceToWeight(j, k, param->maxWeight, param->minWeight) << std::endl;
					}
				}
				for (int j = 0; j < param->nOutput; j++) {
					for (int k = 0; k < param->nHide; k++) {
						arrayHO->EraseCell(j,k);
					}
				}
				for (int j = 0; j < param->nHide; j++) {
					for (int k = 0; k < param->nInput; k++) {
						//std::cout << weight1[j][k] << std::endl;
						arrayIH->WriteCell(j, k, weight1[j][k], weight1[j][k], param->maxWeight, param->minWeight,false);
						//std::cout << arrayIH->ConductanceToWeight(j, k, param->maxWeight, param->minWeight) << std::endl;
					}
				}
				for (int j = 0; j < param->nOutput; j++) {
					for (int k = 0; k < param->nHide; k++) {
						arrayHO->WriteCell(j, k, weight2[j][k], weight2[j][k], param->maxWeight, param->minWeight,false);
					}
				}
				
			 
			 
		  
			} // end of if code
				
			} // end of full-reset code
			
	
		
		
				

	/* track weight distribution */
			
 /*	int positiveweight1 ,positiveweight2;
	int negativeweight1, negativeweight2;
	int zeroweight1, positiveweight2;
	int weightsum1=0, weightsum2=0;
	
	
	 // default distribution tracking -> total weight track 
      	  for (int m=0; m<param->nHide; m++) {
			for (int n=0; n<param->nInput;n++){
			// count polarity of weight 
				if (weight[m][n] > 0) positiveweight1++;
				else if (weight[m][n] ==0) zeroweight1++;
				else negativeweight1++;
			// see if polarity of total sum of weight accords with the maximum polarity count of individual weight 
				weightsum1 += weight1[m][n];
				cout << (weightsum1>0) << " " << (positiveweight1>negativeweight1);
		              
				
		
	  }
	  }  // weightIH
			
			for (int m=0; m<param->nOutput; m++) {
			for (int n=0; n<param->nHide;n++){
			// count polarity of weight 
				if (weight[m][n] > 0) positiveweight2++;
				else if (weight[m][n] ==0) zeroweight2++;
				else negativeweight2++;
			// see if polarity of total sum of weight accords with the maximum polarity count of individual weight 
				weightsum2 += weight2[m][n];
				cout << (weightsum2>0) << " " << (positiveweight2>negativeweight2);
		              
				
		
	  }
	  }  // weightHO
			
			
	ofstream weightdis;
	weightdis.open("weightdistribution.csv",std::ios_base::app);  
	weightdis << epochcount << ", " << batchSize << ", " << batchSize+numTrain*(epochcount-1) <<", "<<(weightsum1>0)<<", "<<(positiveweight1>negativeweight1)<<", "<< (weightsum2>0) << ", " << (positiveweight2>negativeweight2); */
	    	
	}   // end of weight update code for 1 epoch
	
        /* weight infromation tracking */
	// deltaweight, polarity stabilization, momentum existence confirmation 
	double positiveweightmomentumIH=0;
	double positiveweightmomentumIH2=0;
	double negativeweightmomentumIH=0;
		double negativeweightmomentumIH2=0;
	double zeroweightmomentumIH=0;
		double zeroweightmomentumIH2=0;
	double positiveweightIH=0;
	double negativeweightIH=0;
	double zeroweightIH=0;
	double polaritychangecountIH=0;
	double possatIH=0;
		double possatIH2=0;
	double negsatIH=0;
		double negsatIH2=0;
	double zerosatIH=0;
			double zerosatIH2=0;
	double positiveweightmomentumHO=0;
		double positiveweightmomentumHO2=0;
	double negativeweightmomentumHO=0;
		double negativeweightmomentumHO2=0;
	double zeroweightmomentumHO=0;
		double zeroweightmomentumHO2=0;
	double positiveweightHO=0;
	double negativeweightHO=0;
	double zeroweightHO=0;
	double polaritychangecountHO=0;
	double possatHO=0;
			double possatHO2=0;
	double negsatHO=0;
			double negsatHO2=0;
	double zerosatHO=0;
			double zerosatHO2=0;
	double healthyfactorIH=0;
	double healthyfactorHO=0;
	int healthycounterIH=0;
	int healthycounterHO=0;
		for (int m=0; m<param->nHide; m++) {
			for (int n=0; n<param->nInput;n++){	
				// count polarity change
				polaritychangecountIH += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->polaritychange;
				// set polarity to count in next epoch
				if(weight1[m][n]>0) 
				static_cast<AnalogNVM*>(arrayIH->cell[m][n])->previouspolarity=1;
				else if (weight1[m][n]<0) 
				static_cast<AnalogNVM*>(arrayIH->cell[m][n])->previouspolarity=-1;
				else
				static_cast<AnalogNVM*>(arrayIH->cell[m][n])->previouspolarity=0;
				
				if (static_cast<AnalogNVM*>(arrayIH->cell[m][n])->previouspolarity==1) 
				{positiveweightIH++;
				 if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->posstep>static_cast<AnalogNVM*>(arrayIH->cell[m][n])->negstep)
			         positiveweightmomentumIH++;
				  if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->posstep<static_cast<AnalogNVM*>(arrayIH->cell[m][n])->negstep)
			         positiveweightmomentumIH2++;
				 if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->possat>static_cast<AnalogNVM*>(arrayIH->cell[m][n])->negsat)
				 possatIH++;
				 	 if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->possat<static_cast<AnalogNVM*>(arrayIH->cell[m][n])->negsat)
				 possatIH2++;
				}
				else if (static_cast<AnalogNVM*>(arrayIH->cell[m][n])->previouspolarity==-1) 
				{negativeweightIH++;
				 if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->posstep<static_cast<AnalogNVM*>(arrayIH->cell[m][n])->negstep)
			         negativeweightmomentumIH++;
				  if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->posstep>static_cast<AnalogNVM*>(arrayIH->cell[m][n])->negstep)
			         negativeweightmomentumIH2++;
				 if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->possat<static_cast<AnalogNVM*>(arrayIH->cell[m][n])->negsat)
				 negsatIH++;
				  if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->possat>static_cast<AnalogNVM*>(arrayIH->cell[m][n])->negsat)
				 negsatIH2++;
				}
				else 
				{
				zeroweightIH++;
				if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->posstep>static_cast<AnalogNVM*>(arrayIH->cell[m][n])->negstep)
			         zeroweightmomentumIH++;
					if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->posstep<static_cast<AnalogNVM*>(arrayIH->cell[m][n])->negstep)
			         zeroweightmomentumIH2++;
				 if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->possat>static_cast<AnalogNVM*>(arrayIH->cell[m][n])->negsat)
				 zerosatIH++;	
						 if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->possat<static_cast<AnalogNVM*>(arrayIH->cell[m][n])->negsat)
				 zerosatIH2++;	
				}
				
				static_cast<AnalogNVM*>(arrayIH->cell[m][n])->ResetCounter();
			}
		}
		
		for (int m=0; m<param->nOutput; m++) {
			for (int n=0; n<param->nHide;n++){	
				// count polarity change
				polaritychangecountHO += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->polaritychange;
				// set polarity to count in next epoch
				if(weight2[m][n]>0) 
				static_cast<AnalogNVM*>(arrayHO->cell[m][n])->previouspolarity=1;
				else if (weight2[m][n]<0) 
				static_cast<AnalogNVM*>(arrayHO->cell[m][n])->previouspolarity=-1;
				else
				static_cast<AnalogNVM*>(arrayHO->cell[m][n])->previouspolarity=0;
				
				if (static_cast<AnalogNVM*>(arrayHO->cell[m][n])->previouspolarity==1) 
				{positiveweightHO++;
				 if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->posstep>static_cast<AnalogNVM*>(arrayHO->cell[m][n])->negstep)
			         positiveweightmomentumHO++;
				  if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->posstep<static_cast<AnalogNVM*>(arrayHO->cell[m][n])->negstep)
			         positiveweightmomentumHO2++;
				 if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->possat>static_cast<AnalogNVM*>(arrayHO->cell[m][n])->negsat)
				 possatHO++;
				  if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->possat<static_cast<AnalogNVM*>(arrayHO->cell[m][n])->negsat)
				 possatHO2++;
				}
				else if (static_cast<AnalogNVM*>(arrayHO->cell[m][n])->previouspolarity==-1) 
				{negativeweightHO++;
				 if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->posstep<static_cast<AnalogNVM*>(arrayHO->cell[m][n])->negstep)
			         negativeweightmomentumHO++;
				  if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->posstep>static_cast<AnalogNVM*>(arrayHO->cell[m][n])->negstep)
			         negativeweightmomentumHO2++;
				 if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->possat<static_cast<AnalogNVM*>(arrayHO->cell[m][n])->negsat)
				 negsatHO++;
				  if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->possat>static_cast<AnalogNVM*>(arrayHO->cell[m][n])->negsat)
				 negsatHO2++;
				}
				else 
				{
				zeroweightHO++;
				if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->posstep>static_cast<AnalogNVM*>(arrayHO->cell[m][n])->negstep)
			         zeroweightmomentumIH++;
					if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->posstep<static_cast<AnalogNVM*>(arrayHO->cell[m][n])->negstep)
			         zeroweightmomentumIH2++;
				 if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->possat>static_cast<AnalogNVM*>(arrayHO->cell[m][n])->negsat)
				 zerosatHO++;	
					 if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->possat<static_cast<AnalogNVM*>(arrayHO->cell[m][n])->negsat)
				 zerosatHO2++;	
				}
			}
		}
		

		cout<<"polaritychange"<<" "<<"IH : "<<polaritychangecountIH<<" HO : "<<polaritychangecountHO<<endl;
		cout<<"momentum prediction"<<" "<<"IH : "<<positiveweightmomentumIH/(positiveweightmomentumIH+positiveweightmomentumIH2)<<", "<<negativeweightmomentumIH/(negativeweightmomentumIH+negativeweightmomentumIH2)<<", "<<zeroweightmomentumIH/(zeroweightmomentumIH+zeroweightmomentumIH2)<<endl;
		cout<<"saturation prediction"<<" "<<"IH : "<<possatIH/(possatIH+possatIH2)<<", "<<negsatIH/(negsatIH+negsatIH2)<<", "<<zerosatIH/(zerosatIH+zerosatIH2)<<endl;

		
		cout<<"momentum prediction"<<" "<<"HO : "<<positiveweightmomentumHO/(positiveweightmomentumHO+positiveweightmomentumHO2)<<", "<<negativeweightmomentumHO/(negativeweightmomentumHO+negativeweightmomentumHO2)<<", "<<zeroweightmomentumHO/(zeroweightmomentumHO+zeroweightmomentumHO2)<<endl;
		cout<<"saturation prediction"<<" "<<"HO : "<<possatHO/(possatHO+possatHO2)<<", "<<negsatHO/(negsatHO+negsatHO2)<<", "<<zerosatHO/(zerosatHO+zerosatHO2)<<endl;
						      
	
	// healthy weight, healthy factor
			for (int m=0; m<param->nHide; m++) {
			for (int n=0; n<param->nInput;n++){
			if ((weight1[m][n]!=1)&&(weight1[m][n]!=-1)){
			if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->conductanceGp>8)
			{healthycounterIH++;
			/*healthyfactorIH += (1-static_cast<AnalogNVM*>(arrayIH->cell[m][n])->conductanceGn/static_cast<AnalogNVM*>(arrayIH->cell[m][n])->nmaxConductance)/(weight1[m][n]);
		        else
			healthyfactorIH += (1-static_cast<AnalogNVM*>(arrayIH->cell[m][n])->conductanceGp/static_cast<AnalogNVM*>(arrayIH->cell[m][n])->pmaxConductance)/(weight1[m][n]); */
			healthyfactorIH +=(1-(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->conductanceGn/static_cast<AnalogNVM*>(arrayIH->cell[m][n])->nmaxConductance));	}
			}
			}
			}
			
			for (int m=0; m<param->nOutput; m++) {
			for (int n=0; n<param->nHide;n++){
			if ((weight2[m][n]!=1)&&(weight2[m][n]!=-1)){
			if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->conductanceGp>8)
			{healthycounterHO++;
			/*healthyfactorHO += (1-static_cast<AnalogNVM*>(arrayHO->cell[m][n])->conductanceGn/static_cast<AnalogNVM*>(arrayHO->cell[m][n])->nmaxConductance)/(weight2[m][n]);
		        else
			healthyfactorHO += (1-static_cast<AnalogNVM*>(arrayHO->cell[m][n])->conductanceGp/static_cast<AnalogNVM*>(arrayHO->cell[m][n])->pmaxConductance)/(weight2[m][n]);*/
			healthyfactorHO +=(1-(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->conductanceGn/static_cast<AnalogNVM*>(arrayHO->cell[m][n])->nmaxConductance));}	
			}
			}
			}
				
		cout<<"averagehealtyfactor IH : "<<healthyfactorIH/healthycounterIH<<endl;
		cout<<"averagehealtyfactor HO : "<<healthyfactorHO/healthycounterHO<<endl;
		
	// weight analyzer 
	double GpIH1=0;
		double GpIH2=0;
		double GpIH3=0;
		double GpIH4=0;
		double GpIH5=0;
		double GpIH6=0;
		double GnIH1=0;
		double GnIH2=0;
		double GnIH3=0;
		double GnIH4=0;
		double GnIH5=0;
		double GnIH6=0;
		double totalIH1=0;
		double totalIH2=0;
		double totalIH3=0;
		double totalIH4=0;
		double totalIH5=0;
		double totalIH6=0;
		double GpHO1=0;
		double GpHO2=0;
		double GpHO3=0;
		double GpHO4=0;
		double GpHO5=0;
		double GpHO6=0;
		double GnHO1=0;
		double GnHO2=0;
		double GnHO3=0;
		double GnHO4=0;
		double GnHO5=0;
		double GnHO6=0;
		double totalHO1=0;
		double totalHO2=0;
		double totalHO3=0;
		double totalHO4=0;
		double totalHO5=0;
		double totalHO6=0;
	double countIH1=0;
	double countIH2=0;
		double countIH3=0;
			double countIH4=0;
			double countIH5=0;
			double countIH6=0;
			double countHO1=0;
			double countHO2=0;	
		double countHO3=0;
			double countHO4=0;
			double countHO5=0;
		double countHO6=0;
	
		
		 for (int m=0; m<param->nHide; m++) {
			for (int n=0; n<param->nInput;n++){
				
					if((-1<=weight1[m][n])&&(weight1[m][n]<-0.66666)) 
					{GpIH1 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[0];
					 GnIH1 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[1];
					 totalIH1 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[2]; 
					 countIH1++;
					}
					else if((-0.66666<=weight1[m][n])&&(weight1[m][n]<-0.33333))
					{GpIH2 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[0];
					 GnIH2 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[1];
					 totalIH2 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[2]; 
					 countIH2++;
					}
					else if ((-0.33333<=weight1[m][n])&&(weight1[m][n]<0))
					{GpIH3 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[0];
					 GnIH3 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[1];
					 totalIH3 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[2]; 
					 countIH3++;
					}
					else if ((0<=weight1[m][n])&&(weight1[m][n]<0.33333))
					{GpIH4 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[0];
					 GnIH4 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[1];
					 totalIH4 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[2]; 
					 countIH4++;
					}
					else if ((0.33333<=weight1[m][n])&&(weight1[m][n]<0.66666))
					{GpIH5 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[0];
					 GnIH5 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[1];
					 totalIH5 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[2]; 
					 countIH5++;
					}
					else 
					{GpIH6 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[0];
					 GnIH6 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[1];
					 totalIH6 += static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[2]; 
					 countIH6++;
					}
				}
			}
		
		
		 for (int m=0; m<param->nOutput; m++) {
			for (int n=0; n<param->nHide;n++){
				if((-1<=weight2[m][n])&&(weight2[m][n]<-0.66666)) 
					{GpHO1 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[0];
					 GnHO1 +=static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[1];
					 totalHO1 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[2]; 
					 countHO1++;
					}
					else if((-0.66666<=weight2[m][n])&&(weight2[m][n]<-0.33333))
					{GpHO2 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[0];
					 GnHO2 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[1];
					 totalHO2 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[2]; 
					  countHO2++;
					}
					else if ((-0.33333<=weight2[m][n])&&(weight2[m][n]<0))
					{GpHO3 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[0];
					 GnHO3 +=static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[1];
					 totalHO3 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[2]; 
					  countHO3++;
					}
					else if ((0<=weight2[m][n])&&(weight2[m][n]<0.33333))
					{GpHO4 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[0];
					 GnHO4 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[1];
					 totalHO4 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[2]; 
					  countHO4++;
					}
					else if ((0.33333<=weight2[m][n])&&(weight2[m][n]<0.66666))
					{GpHO5 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[0];
					 GnHO5 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[1];
					 totalHO5 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[2]; 
					  countHO5++;
					}
					else 
					{GpHO6 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[0];
					 GnHO6 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[1];
					 totalHO6 += static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[2]; 
					  countHO6++;
					}
			}
		}
		
	
		cout<<"-1~-0.66666"<<" : "<<"IH "<<GpIH1/countIH1<<", "<<GnIH1/countIH1<<", "<<totalIH1/countIH1<<" HO "<<GpHO1/countHO1<<", "<<GnHO1/countHO1<<", "<<totalHO1/countHO1<<endl;
		cout<<"-0.66666~-0.33333"<<" : "<<"IH "<<GpIH2/countIH2<<", "<<GnIH2/countIH2<<", "<<totalIH2/countIH2<<" HO "<<GpHO2/countHO2<<", "<<GnHO2/countHO2<<", "<<totalHO2/countHO2<<endl;
		cout<<"-0.33333~0"<<" : "<<"IH "<<GpIH3/countIH3<<", "<<GnIH3/countIH3<<", "<<totalIH3/countIH3<<" HO "<<GpHO3/countHO3<<", "<<GnHO3/countHO3<<", "<<totalHO3/countHO3<<endl;
		cout<<"0~0.33333"<<" : "<<"IH "<<GpIH4/countIH4<<", "<<GnIH4/countIH4<<", "<<totalIH4/countIH4<<" HO "<<GpHO4/countHO4<<", "<<GnHO4/countHO4<<", "<<totalHO4/countHO4<<endl;
		cout<<"0.33333~0.66666"<<" : "<<"IH "<<GpIH5/countIH5<<", "<<GnIH5/countIH5<<", "<<totalIH5/countIH5<<" HO "<<GpHO5/countHO5<<", "<<GnHO5/countHO5<<", "<<totalHO5/countHO5<<endl;
		cout<<"0.66666~1"<<" : "<<"IH "<<GpIH6/countIH6<<", "<<GnIH6/countIH6<<", "<<totalIH6/countIH6<<" HO "<<GpHO6/countHO6<<", "<<GnHO6/countHO6<<", "<<totalHO6/countHO6<<endl;
	       
	double countGprange =0;
		double countGpweightrange=0;
		double countGnrange =0;
		double countGnweightrange=0;
		double locationnumberspecifier=0;
		double locationnumberspecifier2=0;
				 for (int m=0; m<param->nHide; m++) {
			for (int n=0; n<param->nInput;n++){
				
				if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[2] == 2)
				{locationnumberspecifier++;}
				if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->weightanalyzer()[2] == 5)
				{locationnumberspecifier2++;}
				
				if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->conductanceGp >= Gth2)
				{countGprange ++;
				 if(weight1[m][n]>=Gth2/10)
				 {countGpweightrange++;}

				}
				if(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->conductanceGn >= Gth2)
				{countGnrange ++;
				 if(weight1[m][n]<=-Gth2/10)
				 {countGnweightrange++;}
				}
			}
				 }
				
					 for (int m=0; m<param->nOutput; m++) {
			for (int n=0; n<param->nHide;n++){
				
				if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[2] == 2)
				{locationnumberspecifier++;}
				if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->weightanalyzer()[2] == 5)
				{locationnumberspecifier2++;}
				
	if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->conductanceGp >= Gth2)
				{countGprange ++;
				 if(weight2[m][n]>= Gth2/10)
				 {countGpweightrange++;}
				}
				if(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->conductanceGn >= Gth2)
				{countGnrange ++;
				 if(weight2[m][n]<= -Gth2/10)
				 {countGnweightrange++;}
				}
			}
				 }
		
		cout<<"P(|w|>=Gth2/pconrange | Gp,Gn>=Gth2/10) = "<< countGpweightrange/countGprange<<", "<<countGnweightrange/countGnrange<<endl;
		cout<<"count [L.N(Gp)+L.N(Gn) = 2] : "<<locationnumberspecifier<<endl;
		cout<<"count [L.N(Gp)+L.N(Gn) = 5] : "<<locationnumberspecifier2<<endl;
		
	/* track weights */
	
	// define name for file & parameters
		char fileIH[5];
		char fileHO[5];
                char nurcase [6];
		char frrcase [6];

        
	// define range of conductance for simplicity //
	double minconGpIH = static_cast<AnalogNVM*>(arrayIH->cell[0][0])->pminConductance;
	double minconGnIH = static_cast<AnalogNVM*>(arrayIH->cell[0][0])->nminConductance;
	double minconGpHO = static_cast<AnalogNVM*>(arrayHO->cell[0][0])->pminConductance;
	double minconGnHO = static_cast<AnalogNVM*>(arrayHO->cell[0][0])->nminConductance;
	double rangeGpIH = static_cast<AnalogNVM*>(arrayIH->cell[0][0])->pmaxConductance - static_cast<AnalogNVM*>(arrayIH->cell[0][0])->pminConductance;
	double rangeGnIH = static_cast<AnalogNVM*>(arrayIH->cell[0][0])->nmaxConductance - static_cast<AnalogNVM*>(arrayIH->cell[0][0])->nminConductance;
	double rangeGpHO = static_cast<AnalogNVM*>(arrayHO->cell[0][0])->pmaxConductance - static_cast<AnalogNVM*>(arrayHO->cell[0][0])->pminConductance;
	double rangeGnHO = static_cast<AnalogNVM*>(arrayHO->cell[0][0])->nmaxConductance - static_cast<AnalogNVM*>(arrayHO->cell[0][0])->nminConductance;
	
        
	
		
	if(param->weighttrack==1){
                                                      	
             												
		for (int m=0; m<param->nHide; m++) {
		  for (int i=0; i<4;i++){
			for (int n=100*i; n<100*(i+1);n++){	
	        
	        sprintf(fileIH, "%d", i);
		sprintf(nurcase, "%d",  param->ReverseUpdate);
		sprintf(frrcase, "%d",  param->RefreshRate);
		
		string filenameA="weightIH";
	        filenameA.append(fileIH);
		 filenameA.append(nurcase);
		 filenameA.append(frrcase);
		
		ofstream readA;
		readA.open(filenameA + ".csv",std::ios_base::app);   		
		readA<<endl;
		readA<<epochcount<<", "<<m<<", "<<n; //write Cell index
		readA <<", "<<weight1[m][n];
		readA <<", "<<(static_cast<AnalogNVM*>(arrayIH->cell[m][n])->conductanceGp - minconGpIH) / rangeGpIH <<", "<< (static_cast<AnalogNVM*>(arrayIH->cell[m][n])->conductanceGn - minconGnIH) /rangeGnIH;
		readA <<", "<<static_cast<AnalogNVM*>(arrayIH->cell[m][n])->upc<<", "<<static_cast<AnalogNVM*>(arrayIH->cell[m][n])->unc<<", "<<static_cast<AnalogNVM*>(arrayIH->cell[m][n])->uzc;
		readA <<", "<<a1[m];
			
			}
		   }
		}
		
		
		
		
				
				
		ofstream readB;
	        readB.open("weightHO.csv",std::ios_base::app);    
		
				
		for (int m=0; m<param->nOutput; m++) {
		for (int i=0; i<4; i++){
		for (int n=25*i; n<25*(i+1);n++){
			
		
	        sprintf(fileHO, "%d", i);
		sprintf(nurcase, "%d",  param->ReverseUpdate);
		sprintf(frrcase, "%d",  param->RefreshRate);
		
		string filenameB="weightHO";
	        filenameB.append(fileHO);
		 filenameB.append(nurcase);
		 filenameB.append(frrcase);
	
		ofstream readB;
			
		readB.open(filenameB + ".csv",std::ios_base::app);  			
		readB << endl;		
		readB <<epochcount<<", "<<m<<", "<<n; // write cell index
		readB <<", "<<weight2[m][n];
	        readB <<", "<<(static_cast<AnalogNVM*>(arrayHO->cell[m][n])->conductanceGp -minconGpHO)/ rangeGpHO<<", "<< (static_cast<AnalogNVM*>(arrayHO->cell[m][n])->conductanceGn - minconGnHO) / rangeGnHO;
		readB <<", "<<static_cast<AnalogNVM*>(arrayHO->cell[m][n])->upc<<", "<<static_cast<AnalogNVM*>(arrayHO->cell[m][n])->unc<<", "<<static_cast<AnalogNVM*>(arrayHO->cell[m][n])->uzc;
	        readB <<", "<<a2[m];
			
		}
		}
		}
		

	} // end of weight tracking code
	
		
	
    }  // end of interepoch code (default -> iterate once)
}  // end of Train function
double SGD(double gradient, double learning_rate){
    return -learning_rate * gradient; 
}

double Momentum(double gradient, double learning_rate, double momentumPrev, double GAMA){
    double momentumNow; 
    momentumNow = GAMA*momentumPrev + learning_rate*gradient;
    return -momentumNow;
}

double Adagrad(double gradient, double learning_rate, double gradSquare, double EPSILON){
    return -learning_rate/(sqrt(gradSquare)+EPSILON)*gradient;
}

double RMSprop(double gradient, double learning_rate, double gradSquarePrev,double GAMA, double EPSILON){
    double gradSquareNow;
    gradSquareNow = GAMA*gradSquarePrev+(1-GAMA)*pow(gradient,2);
    return -learning_rate/(sqrt(gradSquareNow)+EPSILON)*gradient;
}

double Adam(double gradient, double learning_rate, double momentumPrev, double velocityPrev, double BETA1, double BETA2, double EPSILON){
    double mt = BETA1*momentumPrev+(1-BETA1)*gradient;
    double vt = BETA2*velocityPrev+(1-BETA2)*pow(gradient,2);
    // correct the bias
    mt = mt/(1-BETA1);
    vt = vt/(1-BETA2);
    return -learning_rate*mt/(sqrt(vt)+EPSILON);
}
