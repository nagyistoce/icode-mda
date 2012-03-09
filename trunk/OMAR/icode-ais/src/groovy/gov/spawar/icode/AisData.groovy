/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package gov.spawar.icode

import com.vividsolutions.jts.geom.Coordinate
import com.vividsolutions.jts.geom.GeometryFactory
import com.vividsolutions.jts.geom.PrecisionModel
import java.text.SimpleDateFormat

/**
 *
 * @author sparta
 */
class AisData {
    
     static def load()
  {
    def geometryFactory = new GeometryFactory(new PrecisionModel(), 4326)

    Ais.withTransaction {
      def istream = AisData.class.getResourceAsStream('aisData.csv')

      istream?.toCsvReader([skipLines: 1]).eachLine {  tokens ->

        def ais = new Ais()
        def location = new Location()
        
        SimpleDateFormat formatter;  //07-MAY-07 11.41.56 AM
        formatter = new SimpleDateFormat("yy-MMM-dd hh.mm.ss a");  

        // CITY_NAME,COUNTRY,POP,CAP,LONGITUDE,LATITUDE

        ais.with {
          mmsi = tokens[0] as Integer
          navStatus  = tokens[1] as Integer
          rateOfTurn = tokens[2]  as Float
          speedOverGround = tokens[3] as Float
          posAccuracy = tokens[4] as Double 
          courseOverGround = tokens[7] as Double
          trueHeading    = tokens[8] as Double 
          IMO = tokens[10] as Integer
          callsign = tokens[11]
          vesselName = tokens[12]
          vesselType = tokens[13] as Integer
          length = tokens[14] as Double
          width = tokens[15] as Double
          eta = (new Date() + 30)
          destination = tokens[23]    
          
        }
        
        location.with{
            
              longitude = tokens[5] as Double
              latitude = tokens[6] as Double
              name = tokens[12]
              date = formatter.parse(tokens[9]);  
              //System.out.println("NewDate-->"+formatter.format(date));  
              
              aisGeom = geometryFactory.createPoint(new Coordinate(longitude, latitude))
          }

        ais.addToLocations(location)
        ais.save()
      }

      istream?.close()
    }
  }
	
}
