/***	Apple Macintosh Developer Technical Support****	SoundInputSample: Sample code to properly record a sound to disk and play it back.****	by Andrew Wulf, Apple Developer Technical Support****	File:		SoundInputSample.c****	Copyright � 1996 Apple Computer, Inc.**	All rights reserved.****	You may incorporate this sample code into your applications without**	restriction, though the sample code has been provided "AS IS" and the**	responsibility for its operation is 100% yours.  However, what you are**	not permitted to do is to redistribute the source as "DSC Sample Code"**	after having made changes. If you're going to re-distribute the source,**	we require that you make it clear in the source that the code was**	descended from Apple Sample Code, but that you've made changes.*/#include <Quickdraw.h>#include <Windows.h>#include <Dialogs.h>#include <Files.h>#include <StandardFile.h>#include <Packages.h>#include <OSEvents.h>#include <Memory.h>#include <Sound.h>	#include <SoundInput.h>#include <SoundComponents.h>#include <OSUtils.h>#include <ToolUtils.h>#include <Types.h>#include <TextUtils.h>#include <SegLoad.h>#include <Gestalt.h>#include <AIFF.h>#include <stdio.h>#include <strings.h>#if PRAGMA_ALIGN_SUPPORTED#pragma options align=mac68k#endiftypedef struct	{	short numberOfRatesSupported;	unsigned long** ratesSupported;	} SampleRatesData;typedef struct	{	short numberOfSizesSupported;	short** sizesSupported;	} SampleSizesData;#if PRAGMA_ALIGN_SUPPORTED#pragma options align=reset#endif#if ((!defined(__MWERKS__))&&(!defined(THINK_C)))	QDGlobals qd;#endifvoid ShowErrAndQuit(char*text, short err);void ShowErr(char*text, short err);void main()	{	short 				i;	OSErr 				err;	long 				soundAttr 				= 0;	SndChannelPtr 		mySoundChannel;	StandardFileReply 	fileReply;	short 				myFRefNum;	long 				mySIRefNum;	SampleSizesData 	sampleSizesInfo 		= { 0, 0 };	SampleRatesData 	sampleRatesInfo 		= { 0, 0 };	OSType 				previousCompressionType;	short 				previousTwosComplement;	short 				previousChannels;	unsigned long		previousSampleRate;	short 				previousSampleSize;	Fixed 				previousGain;	short 				previousVoxRecord[2];	short 				previousVoxStop[3];	OSType 				noCompression 			= NoneType;	OSType 				compressionSetting 		= noCompression;	short 				twosComplementSetting 	= 1;	short 				numChannels 			= 1;	unsigned long 		sampleRate;	short 				sampleSize;	long 				soundDuration 			= 4*1000; 	// in milliseconds	Fixed 				inputGain 				= 0x00010000L;	// = 1.0, can be up to 1.5	short 				voxRecord[2] 			= { 0, 0 };	short 				voxStop[3] 				= { 0, 0, 0 };		SPB 				soundParamBlock;	ParamBlockRec 		paramBlock;		DialogPtr 			infoDialog				= 0;	InitGraf(&qd.thePort);	FlushEvents(everyEvent, 0);	InitWindows();	InitDialogs(0);	InitCursor();		// check gestalt to see if we have sound input support		err = Gestalt(gestaltSoundAttr, &soundAttr);		if (err || (soundAttr & (1L<<gestaltSoundIOMgrPresent))==0			|| (soundAttr & (1L<<gestaltBuiltInSoundInput))==0)		ShowErrAndQuit("Insufficient sound input support", 0);		// Make a file to save sound into		StandardPutFile("\pPick a file to record to:", "\p", &fileReply);	if (! fileReply.sfGood)		ShowErrAndQuit("Goodbye", 0);	FSpCreate(&fileReply.sfFile, 'SCPL', 'AIFC', 0);	// a SoundApp creator & filetype	err = FSpOpenDF(&fileReply.sfFile, fsRdWrPerm, &myFRefNum);	if (err != noErr)		ShowErrAndQuit("FSpOpenDF", err);					// Open the default sound input device		err = SPBOpenDevice("\p", siWritePermission, &mySIRefNum);	if (err != noErr)		ShowErrAndQuit("SPBOpenDevice", err);			// Get the sample sizes available		err = SPBGetDeviceInfo(mySIRefNum, siSampleSizeAvailable, (Ptr) &sampleSizesInfo);	if (err != noErr)		ShowErrAndQuit("SPBGetDeviceInfo with siSampleSizeAvailable", err);		// Get the sample rates supported	err = SPBGetDeviceInfo(mySIRefNum, siSampleRateAvailable, (Ptr) &sampleRatesInfo);	if (err != noErr)		ShowErrAndQuit("SPBGetDeviceInfo with siSampleRateAvailable", err);		// get the current settings of the sound input device; to be friendly, we'll set them	// back after we're through		err = SPBGetDeviceInfo(mySIRefNum, siInputGain, (Ptr) &previousGain);	// ignore error, input device isn't required to support this	err = SPBGetDeviceInfo(mySIRefNum, siVoxRecordInfo, (Ptr) previousVoxRecord);	// ignore error, input device isn't required to support this	err = SPBGetDeviceInfo(mySIRefNum, siVoxStopInfo, (Ptr) previousVoxStop);	// ignore error, input device isn't required to support this	err = SPBGetDeviceInfo(mySIRefNum, siCompressionType, (Ptr) &previousCompressionType);	if (err != noErr)		ShowErrAndQuit("SPBGetDeviceInfo with siCompressionType", err);	err = SPBGetDeviceInfo(mySIRefNum, siNumberChannels, (Ptr) &previousChannels);	if (err != noErr)		ShowErrAndQuit("SPBGetDeviceInfo with siNumberChannels", err);	err = SPBGetDeviceInfo(mySIRefNum, siSampleRate, (Ptr) &previousSampleRate);	if (err != noErr)		ShowErrAndQuit("SPBGetDeviceInfo with siSampleRate", err);	err = SPBGetDeviceInfo(mySIRefNum, siSampleSize, (Ptr) &previousSampleSize);	if (err != noErr)		ShowErrAndQuit("SPBGetDeviceInfo with siSampleSize", err);	err = SPBGetDeviceInfo(mySIRefNum, siTwosComplementOnOff, (Ptr) &previousTwosComplement);	if (err != noErr)		ShowErrAndQuit("SPBGetDeviceInfo with siTwosComplementOnOff", err);		// pick the biggest sample size available		sampleSize = 0;	for (i=0; i< sampleSizesInfo.numberOfSizesSupported; i++)		if ((*sampleSizesInfo.sizesSupported)[i] > sampleSize)			sampleSize = (*sampleSizesInfo.sizesSupported)[i];		// MACE compression only supported in 8 bit		if ( compressionSetting == kMace3SubType || compressionSetting == kMace6SubType )		{		sampleSize = 8;		twosComplementSetting = 0;		}		// now pick the best sample rate available, if the count is 0, you get a range of [low, high]	// note the sample rate is actually treated as an "unsigned" fixed value		if (sampleRatesInfo.numberOfRatesSupported == 0)		sampleRate = (*sampleRatesInfo.ratesSupported)[ 1 ];	else		{		sampleRate = 0;		for (i=0; i< sampleRatesInfo.numberOfRatesSupported; i++)			if ((*sampleRatesInfo.ratesSupported)[i] > sampleRate)				sampleRate = (*sampleRatesInfo.ratesSupported)[i];		}		//-------------------------------------------------------------------------------	// now we'll set the parameters we want - this is very important to do, since you	// won't get consistant results otherwise. Never assume you know the state! You	// share the input driver's state with every other application.	//-------------------------------------------------------------------------------		// initialize the compression type to NONE, if you want compression, set it last	// note that not all devices can compress sound during input, and many	// compressions are not available.	err = SPBSetDeviceInfo(mySIRefNum, siCompressionType, (Ptr) &noCompression);	if (err != noErr)		ShowErrAndQuit("SPBSetDeviceInfo with siCompressionType == NONE", err);	err = SPBSetDeviceInfo(mySIRefNum, siNumberChannels, (Ptr) &numChannels);	if (err != noErr)		ShowErrAndQuit("SPBSetDeviceInfo with siNumberChannels", err);	err = SPBSetDeviceInfo(mySIRefNum, siSampleSize, (Ptr) &sampleSize);	if (err != noErr)		ShowErrAndQuit("SPBSetDeviceInfo with siSampleSize", err);	err = SPBSetDeviceInfo(mySIRefNum, siSampleRate, (Ptr) &sampleRate);	if (err != noErr)		ShowErrAndQuit("SPBSetDeviceInfo with siSampleRate", err);	err = SPBSetDeviceInfo(mySIRefNum, siTwosComplementOnOff, (Ptr) &twosComplementSetting);	if (err != noErr)		ShowErrAndQuit("SPBSetDeviceInfo with siTwosComplementOnOff", err);		// you may get err == siInvalidCompression(-223) if it's not supported for input		if ( compressionSetting != noCompression )		{		err = SPBSetDeviceInfo(mySIRefNum, siCompressionType, (Ptr) &compressionSetting);		if (err != noErr)			ShowErrAndQuit("SPBSetDeviceInfo with siCompressionType", err);		}	err = SPBSetDeviceInfo(mySIRefNum, siInputGain, (Ptr) &inputGain);	// ignore error, input device isn't required to support this		err = SPBSetDeviceInfo(mySIRefNum, siVoxRecordInfo, (Ptr) voxRecord);	// ignore error, input device isn't required to support this	err = SPBSetDeviceInfo(mySIRefNum, siVoxStopInfo, (Ptr) voxStop);	// ignore error, input device isn't required to support this	// note that you could use siRecordingQuality to set the rate, sample size & compression	// 'best', 'better' and 'good' quality are defined by the sound input device,	// so use this if you really don't care what you get		// now setup the AIFF file header		err = SetupAIFFHeader(myFRefNum, numChannels, sampleRate, sampleSize,												compressionSetting, 0, 0);	if (err != noErr)		ShowErrAndQuit("SetupAIFFHeader", err);		// actually record the sound to file		infoDialog = GetNewDialog(129, 0L, (WindowPtr)-1 );	ShowWindow( infoDialog );	DrawDialog( infoDialog );		soundParamBlock.inRefNum = mySIRefNum;	soundParamBlock.count = 0;				// after recording, this holds the # bytes recorded	soundParamBlock.milliseconds = soundDuration;	soundParamBlock.completionRoutine = 0;	soundParamBlock.interruptRoutine = 0;	soundParamBlock.userLong = 0;	soundParamBlock.error = 0;	soundParamBlock.unused1 = 0;		err = SPBRecordToFile(myFRefNum, &soundParamBlock, false);	// synchronous	if (err != noErr)		ShowErrAndQuit("Failure at call to SPBRecordToFile", err);		CloseDialog( infoDialog );		// set the input parameters back the way they were		err = SPBSetDeviceInfo(mySIRefNum, siCompressionType, (Ptr) &previousCompressionType);	if (err != noErr)		ShowErrAndQuit("SPBSetDeviceInfo resetting siCompressionType", err);	err = SPBSetDeviceInfo(mySIRefNum, siNumberChannels, (Ptr) &previousChannels);	if (err != noErr)		ShowErrAndQuit("SPBSetDeviceInfo resetting siNumberChannels", err);	err = SPBSetDeviceInfo(mySIRefNum, siSampleRate, (Ptr) &previousSampleRate);	if (err != noErr)		ShowErrAndQuit("SPBSetDeviceInfo resetting siSampleRate", err);	err = SPBSetDeviceInfo(mySIRefNum, siSampleSize, (Ptr) &previousSampleSize);	if (err != noErr)		ShowErrAndQuit("SPBSetDeviceInfo resetting siSampleSize", err);	err = SPBSetDeviceInfo(mySIRefNum, siTwosComplementOnOff, (Ptr) &previousTwosComplement);	if (err != noErr)		ShowErrAndQuit("SPBSetDeviceInfo resetting siTwosComplementOnOff", err);	err = SPBSetDeviceInfo(mySIRefNum, siInputGain, (Ptr) &previousGain);	// ignore error, input device isn't required to support this	err = SPBSetDeviceInfo(mySIRefNum, siVoxRecordInfo, (Ptr) previousVoxRecord);	// ignore error, input device isn't required to support this	err = SPBSetDeviceInfo(mySIRefNum, siVoxStopInfo, (Ptr) previousVoxStop);	// ignore error, input device isn't required to support this	// We're done recording so close the device	err = SPBCloseDevice(mySIRefNum);	if (err != noErr)		ShowErrAndQuit("SPBCloseDevice", err);	 	// rewind sound file to start   	paramBlock.ioParam.ioCompletion = 0;	paramBlock.ioParam.ioRefNum = myFRefNum;	paramBlock.ioParam.ioPosMode = fsFromStart;	paramBlock.ioParam.ioPosOffset = 0;	err = PBSetFPos(&paramBlock, false);	if (err != noErr)		ShowErrAndQuit("PBSetFPos", err);		// now resetup file header with actual count of bytes	err = SetupAIFFHeader(myFRefNum, numChannels, sampleRate, sampleSize,							compressionSetting, soundParamBlock.count, 0);	if (err != noErr)		ShowErrAndQuit("SetupAIFFHeader 2nd time", err);		// Allocate our own sound channel		mySoundChannel = 0;	err = SndNewChannel(&mySoundChannel, sampledSynth, 0, 0);	if (err != noErr)		ShowErrAndQuit("SndNewChannel", err);		// finally play our sound file		err = SndStartFilePlay(mySoundChannel, myFRefNum, 0, 40000L, 0, 0, 0, false);	if (err != noErr)		ShowErrAndQuit("SndStartFilePlay", err);	// dispose of the sound channel		SndDisposeChannel(mySoundChannel, true);	// close the file		FSClose(myFRefNum);		// cleanup		if (sampleSizesInfo.sizesSupported)		DisposeHandle((Handle)sampleSizesInfo.sizesSupported);	if (sampleRatesInfo.ratesSupported)		DisposeHandle((Handle)sampleRatesInfo.ratesSupported);	}void ShowErr(char*text, short err)	{	char s[256];	if (err)		sprintf((char*)s, "Failure at %s, with error = %hd.", text, err);	else		sprintf((char*)s, "%s.", text);	c2pstr(s);	ParamText((Byte*)s, 0, 0, 0);	NoteAlert(128, 0);	}void ShowErrAndQuit(char*text, short err)	{	ShowErr( text, err );	ExitToShell();	}