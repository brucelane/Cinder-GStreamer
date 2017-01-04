#pragma once

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/params/Params.h"

#include "_2RealGStreamerWrapper.h"


using namespace ci;
using namespace ci::app;
using namespace std;
using namespace _2RealGStreamerWrapper;


class cinderGStreamerGUIApp : public App {
public:
	void setup();
	void update();
	void draw();
    
	void mouseDown(MouseEvent event);
	void fileDrop(FileDropEvent event);

private:
	void setupGui();
	void updateGui();
	void open();
	void clearAll();
	void pause();
	void stop();
	void toggleDirection();
	int	 calcTileDivisor(int size);
	int  calcSelectedPlayer(int x, int y);

	static	cinderGStreamerGUIApp*											m_Instance;
	std::vector<std::shared_ptr<GStreamerWrapper> >							m_Players;
	std::vector<ci::gl::TextureRef>											m_VideoTextures;
	ci::params::InterfaceGl													m_Gui;
	ci::Font																m_Font;
	int																		m_iCurrentVideo;
	double																	m_dLastTime;
	float																	m_fSeekPos;
	float																	m_fOldSeekPos;
	float																	m_fSpeed;
	float																	m_fVolume;
	int																		m_iLoopMode;
	int																		m_iTilesDivisor;
	int																		m_iTileWidth;
	int																		m_iTileHeight;
	bool																	m_bUseVideoBuffer;
	bool																	m_bUseAudioBuffer;
};

void cinderGStreamerGUIApp::setup()
{
 		m_Instance = this;
		FILE* f;
		freopen_s(&f, "CON", "w", stdout);

		setWindowSize(800, 600);
		m_dLastTime = 0;
		m_iCurrentVideo = 0;
		m_fSpeed = 1;
		m_fVolume = 1;
		m_iLoopMode = LoopMode::LOOP;
		m_iTilesDivisor = 1;
		m_fSeekPos = m_fOldSeekPos = 0;
		m_bUseVideoBuffer = true;
		m_bUseAudioBuffer = false;

		m_Font = ci::Font("Consolas", 48);
		setupGui();
		gl::enableAlphaBlending();

		std::shared_ptr<GStreamerWrapper> testFile = std::shared_ptr<GStreamerWrapper>(new GStreamerWrapper());
		std::string path = "file:/" + getAppPath().string() + "/data/morph.avi";
		if (testFile->open(path, m_bUseVideoBuffer, m_bUseAudioBuffer))
		{
			m_Players.push_back(testFile);
			m_VideoTextures.push_back(gl::Texture::create(800, 600));
			m_Players.back()->setLoopMode(LOOP);
			m_Players.back()->play();
		}
}

void cinderGStreamerGUIApp::update()
{
	// set playing properties of current video
	if (m_Players.size() <= 0)
		return;

	m_Players[m_iCurrentVideo]->setSpeed(m_fSpeed);
	m_Players[m_iCurrentVideo]->setLoopMode((LoopMode)m_iLoopMode);
	m_Players[m_iCurrentVideo]->setVolume(m_fVolume);

	updateGui();

	// scrubbing through stream
	if (m_fSeekPos != m_fOldSeekPos)
	{
		m_fOldSeekPos = m_fSeekPos;
		m_Players[m_iCurrentVideo]->setPosition(m_fSeekPos);
	}

	m_iTilesDivisor = calcTileDivisor(m_Players.size());
	m_iTileWidth = getWindowWidth() / m_iTilesDivisor;
	m_iTileHeight = getWindowHeight() / m_iTilesDivisor;
}

void cinderGStreamerGUIApp::draw()
{
	Rectf imgRect;
	int posX;
	int posY;

	gl::clear();

	for (int i = 0; i<m_Players.size(); i++)
	{
		m_Players[i]->update();
		if (m_Players[i]->hasVideo() && m_Players[i]->isNewVideoFrame())
		{
			unsigned char* pImg = m_Players[i]->getVideo();
			if (pImg != nullptr)
			{
				m_VideoTextures[i] = gl::Texture::create(ci::Surface(pImg, m_Players[i]->getWidth(), m_Players[i]->getHeight(), m_Players[i]->getWidth() * 3, ci::SurfaceChannelOrder::RGB));
			}
		}

		posX = (i % m_iTilesDivisor) * m_iTileWidth;
		posY = ((int(float(i) / float(m_iTilesDivisor))) % m_iTilesDivisor) * m_iTileHeight;
		Rectf imgRect = Rectf(posX, posY, posX + m_iTileWidth, posY + m_iTileHeight);
		if (m_VideoTextures[i])
			ci::gl::draw(m_VideoTextures[i], imgRect);
	}

	// draw green selection frame
	if (m_Players.size()>0)
	{
		posX = (m_iCurrentVideo % m_iTilesDivisor) * m_iTileWidth;
		posY = ((int(float(m_iCurrentVideo) / float(m_iTilesDivisor))) % m_iTilesDivisor) * m_iTileHeight;
		gl::color(0, 1, 0, 1);
		glLineWidth(3);
		gl::drawStrokedRect(Rectf(posX, posY, posX + m_iTileWidth, posY + m_iTileHeight));
		gl::color(1, 1, 1, 1);
	}

	// draw fps and gui
	std::stringstream str;
	str << getAverageFps();
	gl::drawString(str.str(), vec2(float(getWindowWidth() - 240), 10.0), Color(1, 0, 0), m_Font);
	m_Gui.draw();

}
void cinderGStreamerGUIApp::open()
{
	fs::path moviePath = getOpenFilePath();
	if (!moviePath.empty())
	{
		std::shared_ptr<GStreamerWrapper> fileToLoad = std::shared_ptr<GStreamerWrapper>(new GStreamerWrapper());
		std::string uri = "file:/" + moviePath.string();
		if (fileToLoad->open(uri, m_bUseVideoBuffer, m_bUseAudioBuffer))
		{
			m_Players.push_back(fileToLoad);
			m_VideoTextures.push_back(gl::Texture::create(800, 600));
			m_Players.back()->play();
		}
	}
}

void cinderGStreamerGUIApp::fileDrop(FileDropEvent event)
{
	for (int i = 0; i<event.getFiles().size(); i++)
	{
		std::shared_ptr<GStreamerWrapper> fileToLoad = std::shared_ptr<GStreamerWrapper>(new GStreamerWrapper());
		std::string uri = "file:/" + event.getFile(i).string();

		if (fileToLoad->open(uri, m_bUseVideoBuffer, m_bUseAudioBuffer))
		{
			m_Players.push_back(fileToLoad);
			m_VideoTextures.push_back(gl::Texture::create(800, 600));
			m_Players.back()->play();
		}
	}
}

void cinderGStreamerGUIApp::clearAll()
{
	m_Players.clear();
	m_VideoTextures.clear();
}

void cinderGStreamerGUIApp::pause()
{
	if (m_Players[m_iCurrentVideo]->getState() == PlayState::PLAYING)
		m_Players[m_iCurrentVideo]->pause();
	else
		m_Players[m_iCurrentVideo]->play();
}

void cinderGStreamerGUIApp::stop()
{
	m_Players[m_iCurrentVideo]->stop();
}

void cinderGStreamerGUIApp::toggleDirection()
{
	if (m_Players[m_iCurrentVideo]->getDirection() == PlayDirection::FORWARD)
		m_Players[m_iCurrentVideo]->setDirection(PlayDirection::BACKWARD);
	else
		m_Players[m_iCurrentVideo]->setDirection(PlayDirection::FORWARD);
}

void cinderGStreamerGUIApp::updateGui()
{
	// Video / Audio Infos
	// update dynamic info of video/audio
	std::stringstream strTmp;
	strTmp << "label='file:  " << m_Players[m_iCurrentVideo]->getFileName() << "'";
	m_Gui.setOptions("file", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='frame:  " << m_Players[m_iCurrentVideo]->getCurrentFrameNumber() << "/" << m_Players[m_iCurrentVideo]->getNumberOfFrames() << "'";
	m_Gui.setOptions("frame", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='time:  " << m_Players[m_iCurrentVideo]->getCurrentTimeInMs() << ":" << m_Players[m_iCurrentVideo]->getDurationInMs() << "'";
	m_Gui.setOptions("time", strTmp.str());
	strTmp.clear();	strTmp.str("");
	/*strTmp << "label='video codec: " << m_Players[m_iCurrentVideo]->getVideoCodecName() << "'";
	m_Gui.setOptions("video codec", strTmp.str());*/
	strTmp.clear();	strTmp.str("");
	/*strTmp << "label='audio codec: " << m_Players[m_iCurrentVideo]->getAudioCodecName() << "'";
	m_Gui.setOptions("audio codec", strTmp.str());*/
	strTmp.clear();	strTmp.str("");
	strTmp << "label='width: " << m_Players[m_iCurrentVideo]->getWidth() << "'";
	m_Gui.setOptions("width", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='height: " << m_Players[m_iCurrentVideo]->getHeight() << "'";
	m_Gui.setOptions("height", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='fps: " << m_Players[m_iCurrentVideo]->getFps() << "'";
	m_Gui.setOptions("fps", strTmp.str());
	strTmp.clear();	strTmp.str("");
	//strTmp << "label='bitrate:  " << m_Players[m_iCurrentVideo]->getBitrate() << "'";
	//m_Gui.setOptions("bitrate", strTmp.str());
	strTmp.clear();	strTmp.str("");
	/*strTmp << "label='audio channels:  " << m_Players[m_iCurrentVideo]->getAudioChannels() << "'";
	m_Gui.setOptions("audio channels", strTmp.str());*/

	strTmp.clear();	strTmp.str("");
	strTmp << "label='audio sample rate:  " << m_Players[m_iCurrentVideo]->getAudioSampleRate() << "'";
	m_Gui.setOptions("audio sample rate", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='frame:  " << m_Players[m_iCurrentVideo]->getCurrentFrameNumber() << "/" << m_Players[m_iCurrentVideo]->getNumberOfFrames() << "'";
	m_Gui.setOptions("frame", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='time:  " << m_Players[m_iCurrentVideo]->getCurrentTimeInMs() << "/" << m_Players[m_iCurrentVideo]->getDurationInMs() << "'";
	m_Gui.setOptions("time", strTmp.str());
}

void cinderGStreamerGUIApp::setupGui()
{
	std::stringstream strTmp;
	m_Gui = ci::params::InterfaceGl("GStreamer Player", ci::ivec2(300, 450));

	// Video / Audio Infos
	m_Gui.addButton("open", std::bind(&cinderGStreamerGUIApp::open, this));
	m_Gui.addButton("clear all", std::bind(&cinderGStreamerGUIApp::clearAll, this));
	m_Gui.addParam("manual video buffer", &m_bUseVideoBuffer);
	m_Gui.addParam("manual audio buffer", &m_bUseAudioBuffer);
	m_Gui.addSeparator();
	strTmp << "label='file: '";
	m_Gui.addText("file", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='video codec: '";
	m_Gui.addText("video codec", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='audio codec: '";
	m_Gui.addText("audio codec", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='width: '";
	m_Gui.addText("width", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='height: '";
	m_Gui.addText("height", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='fps: '";
	m_Gui.addText("fps", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='bitrate: '";
	m_Gui.addText("bitrate", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='audio channels: '";
	m_Gui.addText("audio channels", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='audio sample rate: '";
	m_Gui.addText("audio sample rate", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='frame: '";
	m_Gui.addText("frame", strTmp.str());
	strTmp.clear();	strTmp.str("");
	strTmp << "label='time: '";
	m_Gui.addText("time", strTmp.str());
	m_Gui.addSeparator();
	m_Gui.addButton("play/pause", std::bind(&cinderGStreamerGUIApp::pause, this));
	m_Gui.addButton("stop", std::bind(&cinderGStreamerGUIApp::stop, this));
	m_Gui.addButton("toggle direction", std::bind(&cinderGStreamerGUIApp::toggleDirection, this));
	m_Gui.addSeparator();
	m_Gui.addParam("speed", &m_fSpeed, "min=0 max=8.0 step=0.05");
	m_Gui.addParam("0..none, 1..loop, 2..loopBidi", &m_iLoopMode, "min=0 max=2 step=1");
	m_Gui.addParam("seek frame", &m_fSeekPos, "min=0.0 max=1.0 step=0.01");
	m_Gui.addParam("volume", &m_fVolume, "min=0.0 max=1.0 step=0.01");
}

int	cinderGStreamerGUIApp::calcTileDivisor(int size)
{
	float fTmp = sqrt(double(size));
	if ((fTmp - int(fTmp))>0)
		fTmp++;
	return int(fTmp);
}

int cinderGStreamerGUIApp::calcSelectedPlayer(int x, int y)
{
	int selected = int((float(x) / float(m_iTileWidth))) + m_iTilesDivisor * int((float(y) / float(m_iTileHeight)));

	return selected;
}

void cinderGStreamerGUIApp::mouseDown(MouseEvent event)
{
	if (event.isLeft())
	{
		int selected = calcSelectedPlayer(event.getX(), event.getY());
		if (selected < m_Players.size())
			m_iCurrentVideo = selected;
	}
	else if (event.isRight())
	{
		int selected = calcSelectedPlayer(event.getX(), event.getY());
		if (selected < m_Players.size())
		{
			m_iCurrentVideo = selected;
			m_Players.erase(m_Players.begin() + selected);
			m_VideoTextures.erase(m_VideoTextures.begin() + selected);
			if (m_iCurrentVideo >= m_Players.size())
				m_iCurrentVideo = m_Players.size() - 1;
			if (m_iCurrentVideo <= 0)
				m_iCurrentVideo = 0;
		}
	}
}

// This line tells Cinder to actually create the application
CINDER_APP( cinderGStreamerGUIApp, RendererGl )
