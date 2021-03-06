#include "SDIOAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


SDIOAnalyzerSettings::SDIOAnalyzerSettings() :
	mIs4bit(false),
	mClockChannel( UNDEFINED_CHANNEL ),
	mCmdChannel( UNDEFINED_CHANNEL ),
	mDAT0Channel( UNDEFINED_CHANNEL ),
	mDAT1Channel( UNDEFINED_CHANNEL ),
	mDAT2Channel( UNDEFINED_CHANNEL ),
	mDAT3Channel( UNDEFINED_CHANNEL )
{
	mIs4bitInterface.reset( new AnalyzerSettingInterfaceBool() );
	mClockChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mCmdChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mDAT0ChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mDAT1ChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mDAT2ChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mDAT3ChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );

	mIs4bitInterface->SetTitleAndTooltip( "4 bit Bus", "Standard SDIO" );
	mClockChannelInterface->SetTitleAndTooltip( "Clock", "Standard SDIO" );
	mCmdChannelInterface->SetTitleAndTooltip( "Command", "Standard SDIO" );
	mDAT0ChannelInterface->SetTitleAndTooltip( "DAT0", "Standard SDIO" );
	mDAT1ChannelInterface->SetTitleAndTooltip( "DAT1", "Standard SDIO" );
	mDAT2ChannelInterface->SetTitleAndTooltip( "DAT2", "Standard SDIO" );
	mDAT3ChannelInterface->SetTitleAndTooltip( "DAT3", "Standard SDIO" );

	mIs4bitInterface->SetValue( mIs4bit );
	mClockChannelInterface->SetChannel( mClockChannel );
	mCmdChannelInterface->SetChannel( mCmdChannel );
	mDAT0ChannelInterface->SetChannel( mDAT0Channel );
	mDAT1ChannelInterface->SetChannel( mDAT1Channel );
	mDAT2ChannelInterface->SetChannel( mDAT2Channel );
	mDAT3ChannelInterface->SetChannel( mDAT3Channel );

	mClockChannelInterface->SetSelectionOfNoneIsAllowed( false );
	mCmdChannelInterface->SetSelectionOfNoneIsAllowed( false );
	mDAT0ChannelInterface->SetSelectionOfNoneIsAllowed( false );
	mDAT1ChannelInterface->SetSelectionOfNoneIsAllowed( true );
	mDAT2ChannelInterface->SetSelectionOfNoneIsAllowed( true );
	mDAT3ChannelInterface->SetSelectionOfNoneIsAllowed( true );


	AddInterface( mIs4bitInterface.get() );
	AddInterface( mClockChannelInterface.get() );
	AddInterface( mCmdChannelInterface.get() );
	AddInterface( mDAT0ChannelInterface.get() );
	AddInterface( mDAT1ChannelInterface.get() );
	AddInterface( mDAT2ChannelInterface.get() );
	AddInterface( mDAT3ChannelInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();

	AddChannel( mClockChannel, "Clock", false );
	AddChannel( mCmdChannel, "Command", false );
	AddChannel( mDAT0Channel, "DAT0", false );
	AddChannel( mDAT1Channel, "DAT1", false );
	AddChannel( mDAT2Channel, "DAT2", false );
	AddChannel( mDAT3Channel, "DAT3", false );
}

SDIOAnalyzerSettings::~SDIOAnalyzerSettings()
{
}

bool SDIOAnalyzerSettings::SetSettingsFromInterfaces()
{
	// check channel selection
	{
		bool is4bit;
		Channel d0, d1, d2, d3;

		is4bit = mIs4bitInterface->GetValue();
		d0 = mDAT0ChannelInterface->GetChannel();
        d1 = mDAT1ChannelInterface->GetChannel();
        d2 = mDAT2ChannelInterface->GetChannel();
        d3 = mDAT3ChannelInterface->GetChannel();

		if (is4bit)
		{

			if (d1 == UNDEFINED_CHANNEL || d2 == UNDEFINED_CHANNEL || d3 == UNDEFINED_CHANNEL)
			{
				SetErrorText("Error: 4-bit wide data requires 4 data channels selected.");
				return false;
			}
		}
		else {
			if (d1 != UNDEFINED_CHANNEL || d2 != UNDEFINED_CHANNEL || d3 != UNDEFINED_CHANNEL)
			{
				SetErrorText("Error: 1-bit wide data requires just DAT0 channel selected.");
				return false;
			}
		}
		std::vector<Channel> channels;
		channels.push_back(d0);
		channels.push_back(d1);
		channels.push_back(d2);
		channels.push_back(d3);
		channels.push_back(mClockChannelInterface->GetChannel());
		channels.push_back(mCmdChannelInterface->GetChannel());

		if (AnalyzerHelpers::DoChannelsOverlap(channels.data(), channels.size()) == true)
		{
			SetErrorText("Channel selections must be unique");
			return false;
		}
	}

	mIs4bit = mIs4bitInterface->GetValue();
	mClockChannel = mClockChannelInterface->GetChannel();
	mCmdChannel = mCmdChannelInterface->GetChannel();
	mDAT0Channel = mDAT0ChannelInterface->GetChannel();
	mDAT1Channel = mDAT1ChannelInterface->GetChannel();
	mDAT2Channel = mDAT2ChannelInterface->GetChannel();
	mDAT3Channel = mDAT3ChannelInterface->GetChannel();

	ClearChannels();

	AddChannel( mClockChannel, "Clock", true );
	AddChannel( mCmdChannel, "Command", true );
	AddChannel( mDAT0Channel, "DAT0", true );
	AddChannel( mDAT1Channel, "DAT1", mDAT1Channel != UNDEFINED_CHANNEL);
	AddChannel( mDAT2Channel, "DAT2", mDAT2Channel != UNDEFINED_CHANNEL);
	AddChannel( mDAT3Channel, "DAT3", mDAT3Channel != UNDEFINED_CHANNEL);
	return true;
}

void SDIOAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mIs4bitInterface->SetValue( mIs4bit );
	mClockChannelInterface->SetChannel( mClockChannel );
	mCmdChannelInterface->SetChannel( mCmdChannel );
	mDAT0ChannelInterface->SetChannel( mDAT0Channel );
	mDAT1ChannelInterface->SetChannel( mDAT1Channel );
	mDAT2ChannelInterface->SetChannel( mDAT2Channel );
	mDAT3ChannelInterface->SetChannel( mDAT3Channel );
}

void SDIOAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

    text_archive >> mIs4bit;
	text_archive >> mClockChannel;
	text_archive >> mCmdChannel;
	text_archive >> mDAT0Channel;
	text_archive >> mDAT1Channel;
	text_archive >> mDAT2Channel;
	text_archive >> mDAT3Channel;

	ClearChannels();

	AddChannel( mClockChannel, "Clock", true );
	AddChannel( mCmdChannel, "Cmd", true );
	AddChannel( mDAT0Channel, "DAT0", true );
	AddChannel( mDAT1Channel, "DAT1", mDAT1Channel != UNDEFINED_CHANNEL);
	AddChannel( mDAT2Channel, "DAT2", mDAT2Channel != UNDEFINED_CHANNEL);
	AddChannel( mDAT3Channel, "DAT3", mDAT3Channel != UNDEFINED_CHANNEL);

	UpdateInterfacesFromSettings();
}

const char* SDIOAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

    text_archive << mIs4bit;
	text_archive << mClockChannel;
	text_archive << mCmdChannel;
	text_archive << mDAT0Channel;
	text_archive << mDAT1Channel;
	text_archive << mDAT2Channel;
	text_archive << mDAT3Channel;

	return SetReturnString( text_archive.GetString() );
}
