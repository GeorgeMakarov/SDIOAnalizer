#include "SDIOAnalyzer.h"
#include "SDIOAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

SDIOAnalyzer::SDIOAnalyzer()
:	Analyzer2(),  
	mSettings( new SDIOAnalyzerSettings() ),
	mSimulationInitilized( false ),
	mAlreadyRun(false),
	packetState(WAITING_FOR_CMD)
{
	SetAnalyzerSettings(mSettings.get());
}

SDIOAnalyzer::~SDIOAnalyzer()
{
	KillThread();
}

void SDIOAnalyzer::SetupResults()
{
    mResults.reset(new SDIOAnalyzerResults(this, mSettings.get()));

    SetAnalyzerResults(mResults.get());

    mResults->AddChannelBubblesWillAppearOn(mSettings->mCmdChannel);

	//mResults.reset( new SDIOAnalyzerResults( this, mSettings.get() ) );
	//SetAnalyzerResults( mResults.get() );
	//mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
}

void SDIOAnalyzer::WorkerThread()
{
	
    mAlreadyRun = true;
	mClock = GetAnalyzerChannelData(mSettings->mClockChannel);
	mCmd   = GetAnalyzerChannelData(mSettings->mCmdChannel);
	mDAT0  = GetAnalyzerChannelData(mSettings->mDAT0Channel);
	mDAT1  = mSettings->mDAT1Channel == UNDEFINED_CHANNEL ? nullptr : GetAnalyzerChannelData(mSettings->mDAT1Channel);
	mDAT2  = mSettings->mDAT2Channel == UNDEFINED_CHANNEL ? nullptr : GetAnalyzerChannelData(mSettings->mDAT2Channel);
	mDAT3  = mSettings->mDAT3Channel == UNDEFINED_CHANNEL ? nullptr : GetAnalyzerChannelData(mSettings->mDAT3Channel);

	mClock->AdvanceToNextEdge();
	mCmd->AdvanceToAbsPosition(mClock->GetSampleNumber());
	mDAT0->AdvanceToAbsPosition(mClock->GetSampleNumber());
	if (mDAT1) mDAT1->AdvanceToAbsPosition(mClock->GetSampleNumber());
	if (mDAT2) mDAT2->AdvanceToAbsPosition(mClock->GetSampleNumber());
	if (mDAT3) mDAT3->AdvanceToAbsPosition(mClock->GetSampleNumber());

	packetState = WAITING_FOR_CMD;
	packetCount = 0;

	do
	{
		PacketStateMachine();

		mResults->CommitResults();
		ReportProgress(mClock->GetSampleNumber());
	}
    while (packetState != FINISHED);
}

void SDIOAnalyzer::SyncToSample(U64 sample)
{
	mCmd->AdvanceToAbsPosition(sample);
	mClock->AdvanceToAbsPosition(sample);
	mDAT0->AdvanceToAbsPosition(sample);
	if (mDAT1) mDAT1->AdvanceToAbsPosition(sample);
	if (mDAT2) mDAT2->AdvanceToAbsPosition(sample);
	if (mDAT3) mDAT3->AdvanceToAbsPosition(sample);
}

void SDIOAnalyzer::AddFrame(U64 fromsample, U64 tosample, frameTypes type, U32 data[4])
{
	Frame frame;

	frame.mStartingSampleInclusive = fromsample;
	frame.mEndingSampleInclusive = tosample;
	frame.mFlags = 0;
	frame.mData1 = ((U64)data[0] << 32) | data[1];
	frame.mData2 = ((U64)data[2] << 32) | data[2];
	frame.mType = type;
	mResults->AddFrame(frame);

	//std::cout << "Add Frame " << type << " at " << fromsample << "\n";
}

//Determine whether or not we are in a packet
void SDIOAnalyzer::PacketStateMachine()
{
    U64 prevsample;
    bool donteatedge;
    
    //printf("packetState %d\n", packetState);
	switch (packetState)
	{
	case WAITING_FOR_CMD:
        prevsample = mCmd->GetSampleNumber();
        /*
        if (!mCmd->DoMoreTransitionsExistInCurrentData())
        {
            printf("no more cmds at %llu\n", prevsample);
            packetState = FINISHED;
            break;
        }
         */
        // find next edge of cmd
        //
		mCmd->AdvanceToNextEdge();
        sample = mCmd->GetSampleNumber();
            
        if (sample < prevsample)
        {
            //printf("Moved backward?\n");
            packetState = FINISHED;
            break;
        }
        prevsample = mClock->GetSampleNumber();
        if (prevsample > sample)
        {
            // if clock is ahead of cmd, can't advance clock
            //printf("Clock ahead of sample?\n");
            packetState = FINISHED;
            break;
        }
        donteatedge = false;

        if (BIT_HIGH == mCmd->GetBitState())
            break;

        // sync clock to cmd
		mClock->AdvanceToAbsPosition(sample);
        if (mClock->GetBitState() == BIT_HIGH) {
            // make sure its a falling edge were at
            mClock->AdvanceToNextEdge();
            sample = mClock->GetSampleNumber();
        }
        else {
            U64 clk1, clk2, clk3;
            
            // it is possible that cmd went low before a low-to-high
            // clock which is valid, and we want to sample at the high-to-low
            // and it is also possible they both went low at the same time
            // so if we look ahead at clk and it goes high before 1/2 its cycle
            // then go forward two clock edges to the the real high-to-low
            //
            clk1 = mClock->GetSampleNumber();
            mClock->AdvanceToNextEdge();           // clock goes low-to-high here
            if (BIT_HIGH != mClock->GetBitState())
                printf("expected high clock\n");
            clk2 = mClock->GetSampleNumber();
            clk3 = mClock->GetSampleOfNextEdge();   // clock goes high to low again here, could be our sample also
            
            // if the edge-to-edge time of the whole clock high period is longer than the distance
            // from cmd going low and the next clock going high (with some fudge taken out) then
            // then need to absorb this clock transition
            //
            if ((3*(clk3 - clk2)/4) < (clk2 - clk1)) {
                // nah, the the clock goes high pretty long after cmd drops, this is a case
                // where cmd and clock dropped at the same time most likely
                //
                donteatedge = true;
                sample = clk1;
            }
            else {
                // yeah absorb a clock and sample on the next low edge
                //
                sample = clk2;
            }
        }
        
        // and sample cmd at the low edge
        mCmd->AdvanceToAbsPosition(sample);

        if (donteatedge) {
            // now go forward to rising edge
            mClock->AdvanceToNextEdge(); // clock goes low-to-high here
        }
        /*
        {
            U64 ss = mClock->GetBitState();
            if (ss != BIT_HIGH)
                printf("wanted high clock\n");
        }
        */
		// from here on in the clock is advanced once to get to the next active edge
        // and cmd is samples, and then the clock advanced to the rising edge
        //
		if (mCmd->GetBitState() == BIT_LOW)
		{
            int i;

            for (i = 0; i < 4; i++)
                uValue[i] = 0;
            valueBits = 0;
            valueIndex = 0;

            //printf("next cmd at %llu\n", sample);
            packetState = PACKET_DIR;
			startSample = sample;
			break;
		}
		// found a back edge, keep going waiting for high-to-low
		break;

	case PACKET_DIR:
		mClock->AdvanceToNextEdge(); // should be high-to-low here
		sample = mClock->GetSampleNumber();
		SyncToSample(sample);
		if (mCmd->GetBitState() == BIT_LOW)
			// a device->host message
			uValue[0] = 0;
		else
			uValue[0] = 1;
        packetDirection = uValue[0];
		mClock->AdvanceToNextEdge(); // and rising here
		//printf("dirbit is %d at sample %d\n", packetDirection, sample);
        sample = mClock->GetSampleNumber();

		AddFrame(startSample, sample, uValue[0] ? FRAME_HOST : FRAME_CARD, uValue);

		// get 6 bits of command
		uValue[0] = 0;
        valueIndex = 0;
        valueBits = 0;
        startSample = sample;
		packetState = GET_UINT;
		packetNextState = CMD;
		packetCount = 6;
		break;

	case CMD:
		sample = mClock->GetSampleNumber();
		SyncToSample(sample);
		//printf("cmd %d\n", uValue[0]);
		AddFrame(startSample, sample, FRAME_CMD, uValue);
		// get 32 bits of argument
		startSample = sample;
        packetCount = 32;

        if (packetDirection)
        {
            //  host->card, 32 bits of argument
            packetNextState = ARG;

            // setup expected response length now based on command
            //
            // CMD2, CMD9 and CMD10 respond with R2 of 136 bits
            // others with R1/3/4/5 which are 48 butes
            //
            respLength = 32;

            switch (uValue[0])
            {
            case 2:
            case 9:
            case 10:
                // R2 response is 136 bits, got 6 cmd 1 stop and 7 crc, leaves 122 bits of value
                respLength = 122;
                respType = FRAME_RESP_R2;
                break;
            case 8:
                respType = FRAME_RESP_R7;
                break;
            case 41:
                respType = FRAME_RESP_R3;
                break;
            case 55:
                respType = FRAME_RESP_R4;
                break;
            default:
                // R1 is a 48 bit response, got 6 cmd, 1 stop and 7 crc leaved 32 of value
                respType = FRAME_RESP_R1;
                break;
            }
        }
        else
        {
            packetNextState = RESPONSE;
            packetCount = respLength;
            if (packetCount == 0)
            {
                // got a response before a command, so skip this one
                //
                packetState = ARG; // just to force out clocks
                break;
            }
        }
        uValue[0] = 0;
        valueIndex = 0;
        valueBits = 0;
        packetState = GET_UINT;
		break;

	case ARG:
    case RESPONSE:
		sample = mClock->GetSampleNumber();
		SyncToSample(sample);
		//printf("arg=%08X\n", uValue[0]);
		AddFrame(startSample, sample, (packetState == ARG) ? FRAME_ARG : respType, uValue);
		// get 32 bits of argument
		uValue[0] = 0;
        valueIndex = 0;
        valueBits = 0;
		startSample = sample;
		packetState = GET_UINT;
		packetNextState = CRC;
		packetCount = 7;
		break;

	case CRC:
		sample = mClock->GetSampleNumber();
		SyncToSample(sample);
		//printf("crc=%02X\n", uValue[0]);
		AddFrame(startSample, sample, FRAME_CRC, uValue);
		// get 32 bits of argument
		uValue[0] = 0;
        valueIndex = 0;
        valueBits = 0;
        mClock->AdvanceToNextEdge();
		mClock->AdvanceToNextEdge();
		startSample = sample;
		packetState = STOP_BIT;
		break;

	case STOP_BIT:
		sample = mClock->GetSampleNumber();
        SyncToSample(sample);
		packetState = WAITING_FOR_CMD;
		break;

	case GET_UINT:
        if (packetCount == 0)
        {
            packetState = packetNextState;
            break;
        }
		mClock->AdvanceToNextEdge();
		sample = mClock->GetSampleNumber();
		SyncToSample(sample);
		uValue[valueIndex] <<= 1;
		if (mCmd->GetBitState() == BIT_HIGH)
		{
			uValue[valueIndex] |= 1;
		}
		mClock->AdvanceToNextEdge();
        valueBits++;
        if (valueBits >= 32)
        {
            // go to next 32 bit value each 32 bits
            valueBits = 0;
            valueIndex++;
            if (valueIndex > 3)
            {
                // can't happen, get outta here
                packetState = WAITING_FOR_CMD;
                packetCount = 0;
                break;
            }
        }
		packetCount--;
		//std::cout << packetState << " uv= " << uValue[0] << " " << packetCount << " left\n";
		if (packetCount == 0)
			packetState = packetNextState;
		break;

	default:
		//printf("AAAAA %d\n", packetState);
		break;
	}
}

bool SDIOAnalyzer::NeedsRerun()
{
	return !mAlreadyRun;
}

U32 SDIOAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
    if (!mSimulationInitilized)
        mSimulationDataGenerator.Initialize(device_sample_rate, &*mSettings);

    return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 SDIOAnalyzer::GetMinimumSampleRateHz()
{
	return 25000;
}

const char* SDIOAnalyzer::GetAnalyzerName() const
{
	return "SDIO";
}

const char* GetAnalyzerName()
{
	return "SDIO";
}

Analyzer* CreateAnalyzer()
{
	return new SDIOAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}