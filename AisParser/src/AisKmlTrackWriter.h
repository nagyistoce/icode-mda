#ifndef AisKmlTrackWriter_h
#define AisKmlTrackWriter_h

#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <vector>

#include <AisWriter.h>
#include <AisMessage.h>
//#include <AisTrack.h>
//#include <AisCoordinate.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

//<?xml version="1.0" encoding="UTF-8"?>
//<kml xmlns="http://www.opengis.net/kml/2.2">
//  <Document>
//    <name>Paths</name>
//    <description>Examples of paths. Note that the tessellate tag is by default
//      set to 0. If you want to create tessellated lines, they must be authored
//      (or edited) directly in KML.
//        <![CDATA[
//          <h1>CDATA Tags are useful!</h1>
//          <p><font color="red">Text is <i>more readable</i> and 
//          <b>easier to write</b> when you can avoid using entity 
//          references.</font></p>
//        ]]></description>
//    <Style id="yellowLineGreenPoly">
//      <LineStyle>
//        <color>7f00ffff</color>
//        <width>4</width>
//      </LineStyle>
//      <PolyStyle>
//        <color>7f00ff00</color>
//      </PolyStyle>
//    </Style>
//    <Placemark>
//      <name>Absolute Extruded</name>
//      <description>Transparent green wall with yellow outlines</description>
//      <styleUrl>#yellowLineGreenPoly</styleUrl>
//      <LineString>
//        <extrude>1</extrude>
//        <tessellate>1</tessellate>
//        <altitudeMode>absolute</altitudeMode>
//        <coordinates> -112.2550785337791,36.07954952145647,2357
//          -112.2549277039738,36.08117083492122,2357
//          -112.2552505069063,36.08260761307279,2357
//          -112.2564540158376,36.08395660588506,2357
//          -112.2580238976449,36.08511401044813,2357
//          -112.2595218489022,36.08584355239394,2357
//          -112.2608216347552,36.08612634548589,2357
//          -112.262073428656,36.08626019085147,2357
//          -112.2633204928495,36.08621519860091,2357
//          -112.2644963846444,36.08627897945274,2357
//          -112.2656969554589,36.08649599090644,2357 
//        </coordinates>
//      </LineString>
//    </Placemark>
//  </Document>
//</kml>


class AisCoordinate
{
public:
	AisCoordinate()
	{
		m_lat = 0;
		m_lon = 0;
		m_timestamp = 0;
	}
	
	AisCoordinate(double lat, double lon, int timestamp ){
		m_lat = lat;
		m_lon = lon;
		m_timestamp = timestamp;
	}

	double m_lat;
	double m_lon;
	int m_timestamp;
};


class AisTrack{
public:
	AisTrack(const AisMessage &m)
	{
		//hasStaticInformation = false;
		m_mmsi = m.getMMSI();
		addData(m);
	}
	
	void addStaticMessageInfo(const AisMessage &m)
	{
		for(int i=0; i<m_messages.size(); i++)
		{
			if(m.equalWithExceptionOfTime(m_messages[i]))
			{
				return;
			}
		}

		//if it gets here it was not found in current track, so add it
		m_messages.push_back(m);
	}

	void addData(const AisMessage &message) 
	{
		if((message.getMESSAGETYPE() == 5) || (message.getMESSAGETYPE() == 24))
		{
			addStaticMessageInfo(message);
		}
		else
		{
			m_aisCoords.push_back(AisCoordinate(message.getLAT(), message.getLON(), message.getDATETIME()));
		}
	}
	
	static bool dateCompare(const AisCoordinate& lhs, const AisCoordinate& rhs)
	{
		return lhs.m_timestamp < rhs.m_timestamp;
	}

	void sortByDateTime()
	{
		std::sort(m_aisCoords.begin(), m_aisCoords.end(), dateCompare);
	}

	double getMMSI() const{
		return m_mmsi;
	}

	double getLAT() const
	{
		if(m_aisCoords.size() > 0)
		{
			return m_aisCoords[m_aisCoords.size()-1].m_lat;
		}
		else
		{
			return -999;
		}
	}

	double getLON() const
	{
		if(m_aisCoords.size() > 0)
		{
			return m_aisCoords[m_aisCoords.size()-1].m_lon;
		}
		else
		{
			return -999;
		}
	}

	AisCoordinate& operator[] (unsigned int idx)
	{
		return m_aisCoords[idx];
	}

	unsigned int size()
	{
		return m_aisCoords.size();
	}

	std::vector<AisMessage> m_messages;
private:
	double m_mmsi;
	std::vector<AisCoordinate> m_aisCoords;
};

class AisTrackSet{
public:
	AisTrackSet()
	{}

	~AisTrackSet(){}

	void add(const AisMessage& m)
	{
		//check if any tracks mmmsi == m.getMMSI();
		if(!alreadyHaveShip(m))
		{
			aisTracks.push_back(AisTrack(m));
		}
	}

	//assumes a height of 0
	//returns in units of meters squared
	//uses wgs84
	double distanceSquaredBetweenPoints(double lat1, double lon1, double lat2, double lon2)
	{
		//http://en.wikipedia.org/wiki/Geodetic_system WGS84
		double theEccentricitySquared = .00669437999014;
		double theA = 6378137.0;// meters
		double radiansPerDegree = 3.141592653589793238462643/180;

		double sin_latitude1 = std::sin(lat1*radiansPerDegree);
		double cos_latitude1 = std::cos(lat1*radiansPerDegree);
		double N1 = theA / sqrt( 1.0 - theEccentricitySquared*sin_latitude1*sin_latitude1);
		double x1 = (N1)*cos_latitude1*std::cos(lon1*radiansPerDegree);
		double y1 = (N1)*cos_latitude1*std::sin(lon1*radiansPerDegree);
		double z1 = (N1*(1-theEccentricitySquared))*sin_latitude1;


		double sin_latitude2 = std::sin(lat2*radiansPerDegree);
		double cos_latitude2 = std::cos(lat2*radiansPerDegree);
		double N = theA / sqrt( 1.0 - theEccentricitySquared*sin_latitude2*sin_latitude2);
		double x2 = (N)*cos_latitude2*std::cos(lon2*radiansPerDegree);
		double y2 = (N)*cos_latitude2*std::sin(lon2*radiansPerDegree);
		double z2 = (N*(1-theEccentricitySquared))*sin_latitude2;

		double magSquared = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2);
		return magSquared;
	}

	bool messageBelongsToCurrentTrack(const AisMessage& m, std::vector<AisTrack>::iterator t)
	{
		if(m.getMMSI() == t->getMMSI())
		{
			if(t->getLAT() != -999 && m.getLAT() != -999 )//&& t->getLON() != -999 && m.getLON() != -999
			{
				//check if distance from the last point added to the track is small. This would imply they come from the same track
				//If the distance is far away, this means it doesn't come from the same track.
				//TODO: The points in the track aren't necessarily given by timestamp (there may be points in the track that are close enough,
				//to think that it is the same track, but the one that we check tells it to start a new track because it is too far away.
				double magSquared = distanceSquaredBetweenPoints(t->getLAT(), t->getLON(), m.getLAT(), m.getLON());
				if(magSquared > 1000000000000) //if distance^2 between points is > (1000km)^2 == if distance > 1000 km
				{
					return false;
				}
			}

			return true;
		}
		
		return false;
	}

	bool alreadyHaveShip(const AisMessage& message)
	{

		std::vector<AisTrack>::iterator itr = aisTracks.begin();

		while(itr != aisTracks.end()){
			if(messageBelongsToCurrentTrack(message, itr))
			{
				itr->addData(message);
				return true;
			}
			itr++;
		}
		return false;
	}

	void replaceBracketsAndAmpersands(std::string &input)
	{
		boost::algorithm::replace_all(input, "<", "&lt;");
		boost::algorithm::replace_all(input, ">", "&gt;");
		boost::algorithm::replace_all(input, "&", "&amp;");
	}

	//requires: trackIdx<aisTracks.size()
	void writeTrack(std::ofstream &of, unsigned int trackIdx)
	{
		if(trackIdx >= aisTracks.size()){
			aisDebug("AisTrackSet::writeTrack the trackIdx exceeds the maximum index of aisTrackSet");
			return;
		}

		string vesselNames;
		for(int staticMessageIdx = 0; staticMessageIdx < aisTracks[trackIdx].m_messages.size(); staticMessageIdx++)
		{
			vesselNames += aisTracks[trackIdx].m_messages[staticMessageIdx].getVESSELNAME() + " - ";
		}

		replaceBracketsAndAmpersands(vesselNames);
		of << "<Placemark>" << endl;
		of << "	<name>" << vesselNames << "</name>" << endl;
		of << "		<description>" << endl;
		of << "		<![CDATA[" << endl;
		of << setprecision(10) << "			MMSI:" << aisTracks[trackIdx].getMMSI() << "<br>" << endl;
		for(int staticMessageIdx = 0; staticMessageIdx < aisTracks[trackIdx].m_messages.size(); staticMessageIdx++)
		{
			of << "			Message Type:" << aisTracks[trackIdx].m_messages[staticMessageIdx].getMESSAGETYPE() << "<br>" << endl;
			of << "			Navigation Status:" << aisTracks[trackIdx].m_messages[staticMessageIdx].getNAVSTATUS() << "<br>" << endl;
			//of << "			ROT:" << aisTracks[trackIdx].m_messages[staticMessageIdx].getROT() << "<br>" << endl;
			//of << "			SOG:" << aisTracks[trackIdx].m_messages[staticMessageIdx].getSOG() << "<br>" << endl;
			//of << "			LON:" << aisTracks[trackIdx].m_messages[staticMessageIdx].getLON() << "<br>" << endl;
			//of << "			LAT:" << aisTracks[trackIdx].m_messages[staticMessageIdx].getLAT() << "<br>" << endl;
			//of << "			COG:" << aisTracks[trackIdx].m_messages[staticMessageIdx].getCOG() << "<br>" << endl;
			//of << "			True Heading:" << aisTracks[trackIdx].m_messages[staticMessageIdx].getTRUE_HEADING() << "<br>" << endl;
			of << "			Date Time:" << aisTracks[trackIdx].m_messages[staticMessageIdx].getDATETIME() << "<br>" << endl;
			of << "			IMO:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getIMO() << "<br>" << endl;
			of << "			Vessel Name:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getVESSELNAME() << "<br>" << endl;
			of << "			Vessel Type Int:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getVESSELTYPEINT() << "<br>" << endl;
			of << "			Ship Length:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getSHIPLENGTH() << "<br>" << endl;
			of << "			Ship Width:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getSHIPWIDTH() << "<br>" << endl;
			of << "			BOW:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getBOW() << "<br>" << endl;
			of << "			STERN:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getSTERN() << "<br>" << endl;
			of << "			PORT:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getPORT() << "<br>" << endl;
			of << "			STARBOARD:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getSTARBOARD() << "<br>" << endl;
			of << "			DRAUGHT:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getDRAUGHT() << "<br>" << endl;
			of << "			Destination:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getDESTINATION() << "<br>" << endl;
			of << "			Callsign:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getCALLSIGN() << "<br>" << endl;
			of << "			Position Accuracy:" << aisTracks[trackIdx].m_messages[staticMessageIdx].getPOSACCURACY() << "<br>" << endl;
			of << "			ETA:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getETA() << "<br>" << endl;
			of << "			Position Fix Type:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getPOSFIXTYPE() << "<br>" << endl;
			of << "			StreamID:" <<  aisTracks[trackIdx].m_messages[staticMessageIdx].getSTREAMID() << "<br>" << endl;
			of << "			-------------------<br>" << endl;
		}
		of << "		]]>" << endl;
		of << "		</description>" << endl;
		of << "	<styleUrl>#yellowLineGreenPoly</styleUrl>" << endl;
		of << "	<LineString>" << endl;
		of << "		<extrude>0</extrude>" << endl;
		of << "		<tessellate>1</tessellate>" << endl;
		of << "		<altitudeMode>clampToGround</altitudeMode>" << endl;
		of << "		<coordinates>" << endl;
		aisTracks[trackIdx].sortByDateTime();
		for(int coordIdx = 0; coordIdx < aisTracks[trackIdx].size(); coordIdx++)
		{
			of << setprecision(10) << "			" << aisTracks[trackIdx][coordIdx].m_lon << "," << aisTracks[trackIdx][coordIdx].m_lat << ",0" << endl;
		}
		of << "		</coordinates>" << endl;
		of << "	</LineString>" << endl;
		of << "</Placemark>" << endl;

		//write first with a pin
		if(aisTracks[trackIdx].size() > 0)
		{
			of << "<Placemark>" << endl;
			of << "	<description>" << endl;
			of << "		MMSI:" << aisTracks[trackIdx].getMMSI() << endl;
			of << setprecision(10) << "		Timestamp:" << aisTracks[trackIdx][0].m_timestamp << endl;
			of << "	</description>" << endl;
			of << " <Point>" << endl;
			of << "		<coordinates>" << endl;
			of << setprecision(10) << "			" << aisTracks[trackIdx][0].m_lon << "," << aisTracks[trackIdx][0].m_lat << ",0" << endl;
			of << "		</coordinates>" << endl;
			of << "	</Point>" << endl;
			of << "</Placemark>" << endl;
		}

		//write last with a pin
		if(aisTracks[trackIdx].size() > 1)
		{
			unsigned int lastIdx = aisTracks[trackIdx].size()-1;
			of << "<Placemark>" << endl;
			of << "	<description>" << endl;
			of << "		MMSI:" << aisTracks[trackIdx].getMMSI() << endl;
			of << setprecision(10) << "		Timestamp:" << aisTracks[trackIdx][lastIdx].m_timestamp << endl;
			of << "	</description>" << endl;
			of << " <Point>" << endl;
			of << "		<coordinates>" << endl;
			of << setprecision(10) << "			" << aisTracks[trackIdx][lastIdx ].m_lon << "," << aisTracks[trackIdx][lastIdx ].m_lat << ",0" << endl;
			of << "		</coordinates>" << endl;
			of << "	</Point>" << endl;
			of << "</Placemark>" << endl;
		}

		//for(int coordIdx = 0; coordIdx < aisTracks[trackIdx].size(); coordIdx++)
		//{
		//	of << "<Placemark>" << endl;
		//	of << "	<description>" << endl;
		//	of << "		MMSI:" << aisTracks[trackIdx].getMMSI() << endl;
		//	of << setprecision(10) << "		Timestamp:" << aisTracks[trackIdx][coordIdx].m_timestamp << endl;
		//	of << "	</description>" << endl;
		//	of << " <Point>" << endl;
		//	of << "		<coordinates>" << endl;
		//	of << setprecision(10) << "			" << aisTracks[trackIdx][coordIdx].m_lon << "," << aisTracks[trackIdx][coordIdx].m_lat << ",0" << endl;
		//	of << "		</coordinates>" << endl;
		//	of << "	</Point>" << endl;
		//	of << "</Placemark>" << endl;
		//}
	}

	void writeData(std::ofstream &of)
	{
		for(int trackIdx = 0; trackIdx<aisTracks.size(); trackIdx++)
		{
			writeTrack(of, trackIdx);
		}
	}

	unsigned int size()
	{
		return aisTracks.size();
	}

private:
	std::vector<AisTrack> aisTracks;
};

class AisKmlTrackWriter : public AisWriter{
public:
	AisKmlTrackWriter(std::string filename){
		m_tracksPerFile = 0;
		m_currentPartition = 0;
		m_filename = filename;
	}

	AisKmlTrackWriter(std::string filename, unsigned int tracksPerFile){
		m_filename = filename;
		m_tracksPerFile = tracksPerFile;
		m_currentPartition = 0;
	}
	~AisKmlTrackWriter(){
		closeFile();
	}
	
	void closeFile()
	{
		if(of.is_open()){
			of << "</Document>" << endl;
			of << "</kml>" << endl;
			of.close();
		}
	}

	bool isReady(){
		return (m_filename != "") && (m_tracksPerFile >= 0);
	}

	bool writeEntry(const AisMessage& message)
	{
		m_trackSet.add(message);
		return true;
	}

	void writeToFile(){
		unsigned int currentTrack = 0;
		while(currentTrack < m_trackSet.size())
		{
			std::string currentFilename = m_filename + ".p" + boost::lexical_cast<string>(m_currentPartition++) + ".kml";
			aisDebug("Writing to file: " << currentFilename );
			of.open(currentFilename, std::ios::out);
		
			if(of.is_open())
			{
				of << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
				of << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">" << endl;
				of << "<Document>" << endl;
				of << "<Style id=\"yellowLineGreenPoly\">" << endl;
				of << "	<LineStyle>" << endl;
				of << "		<color>7f00ffff</color>" << endl;
				of << "		<width>4</width>" << endl;
				of << "	</LineStyle>" << endl;
				of << "	<PolyStyle>" << endl;
				of << "		<color>7f00ff00</color>" << endl;
				of << "	</PolyStyle>" << endl;
				of << "</Style>" << endl;
			}
			else
			{
				aisDebug("kml file writer not ready");
			}

			for(unsigned int trackCount = 0; ((m_tracksPerFile == 0) || (trackCount < m_tracksPerFile)) && (currentTrack < m_trackSet.size()); trackCount++){
				m_trackSet.writeTrack(of, currentTrack++);
			}
			closeFile();
		}
	}

private:
	std::ofstream of;
	std::string m_filename;
	unsigned int m_tracksPerFile;
	unsigned int m_currentPartition;
	AisTrackSet m_trackSet;
};

#endif
