package gov.spawar.icode

import geoscript.workspace.PostGIS
import geoscript.layer.Layer
import geoscript.render.Draw
import geoscript.geom.Bounds
import geoscript.feature.Field
import geoscript.style.Shape
import geoscript.style.Fill
import geoscript.style.Label
import geoscript.proj.Projection
import org.apache.commons.collections.map.CaseInsensitiveMap

class TimeLocationService
{

  static transactional = false

  def getMap( def params, def response )
  {
    def wmsParams = new CaseInsensitiveMap( params )
    def postgis = new PostGIS( [user: 'postgres'], 'geodata-dev' )
    def coords = wmsParams.bbox.split( ',' ).collect { it.toDouble() }

    /*
    def sql = """
    select a.id as id, vessel_name, date as last_known_time, ais_geom as last_known_position
    from ais a, location l
    where a.id=l.ais_id
    and st_within(ais_geom, st_makeenvelope( ${coords[0]}, ${coords[1]}, ${coords[2]}, ${coords[3]}, 4326 ))
   """
   */

    def sql = """
    select x.id as id, vessel_name, last_known_time, last_known_position
    from ais x, (
      select distinct a.ais_id, b.date as last_known_time, ais_geom as last_known_position
      from location a
      inner join (
        select ais_id, max(date) as date
        from location
        where st_within(ais_geom, st_makeenvelope( ${coords[0]}, ${coords[1]}, ${coords[2]}, ${coords[3]}, 4326 ))
        group by ais_id
      ) b
      on a.ais_id=b.ais_id
      and a.date=b.date
    ) as y
    where x.id=y.ais_id
    order by vessel_name
    """

    def layer = postgis.addSqlQuery(
            Layer.newname(),
            sql,
            new Field( 'last_known_position', 'POINT', 'EPSG:4326' ),
            ['id']
    )
    def proj = new Projection( wmsParams.srs )
    def bounds = new Bounds( coords[0], coords[1], coords[2], coords[3], proj )
    def size = [wmsParams.width.toInteger(), wmsParams.height.toInteger()]
    def output = response.outputStream

    response.contentType = wmsParams.format
    layer.style = new Fill( color: '#000000', opacity: 0 ) + new Shape( color: '#FF0000', type: 'circle', size: 5 ) + new Label(property: 'vessel_name')
    Draw.draw( layer, bounds, size, output, wmsParams.format - 'image/' )
    postgis?.close()
  }
}


