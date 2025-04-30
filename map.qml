import QtQuick
import QtLocation
import QtPositioning

Item {
    id: root
    anchors.fill: parent

    Plugin {
        id: mapPlugin
        name: "osm" // OpenStreetMap

        // OSM configuration
        PluginParameter { name: "osm.useragent"; value: "AsteriaApp" }
        PluginParameter { name: "osm.mapping.providersrepository.disabled"; value: "true" }
        PluginParameter { name: "osm.mapping.highdpi_tiles"; value: "true" }
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(37.9838, 23.7275) // Default to Athens, Greece
        zoomLevel: 10

        // Enable interactive features
        copyrightsVisible: false

        // Add a marker when clicked
        MapQuickItem {
            id: marker
            visible: false
            anchorPoint.x: markerImage.width/2
            anchorPoint.y: markerImage.height

            sourceItem: Rectangle {
                id: markerImage
                width: 20
                height: 20
                color: "red"
                border.color: "black"
                border.width: 2
                radius: 10
            }
        }
    }

    // Custom implementation for dragging
    MouseArea {
        id: mouseArea
        anchors.fill: parent

        property bool isDragging: false
        property point lastPosition

        onPressed: {
            isDragging = false
            lastPosition = Qt.point(mouse.x, mouse.y)
        }

        onPositionChanged: {
            if ((Math.abs(mouse.x - lastPosition.x) > 10 ||
                 Math.abs(mouse.y - lastPosition.y) > 10) && !isDragging) {
                isDragging = true
            }

            if (isDragging) {
                var dx = mouse.x - lastPosition.x
                var dy = mouse.y - lastPosition.y

                var newCenter = map.toCoordinate(
                    Qt.point(map.width/2 - dx, map.height/2 - dy),
                    false /* clipToViewPort */
                )

                map.center = newCenter
                lastPosition = Qt.point(mouse.x, mouse.y)
            }
        }

        onReleased: {
            if (!isDragging) {
                // This was a click, not a drag
                var coordinate = map.toCoordinate(Qt.point(mouse.x, mouse.y))
                marker.coordinate = coordinate
                marker.visible = true

                // Send coordinates to C++
                mapDialog.onMapClicked(coordinate.latitude, coordinate.longitude)
            }
            isDragging = false
        }

        onWheel: function(wheel) {
            if (wheel.angleDelta.y > 0)
                map.zoomLevel += 0.2;
            else
                map.zoomLevel -= 0.2;
        }
    }

    // Add zoom controls
    Column {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 10
        z: 1 // Make sure controls are above the map

        Rectangle {
            width: 40
            height: 40
            color: "#ffffff"
            border.color: "#cccccc"
            radius: 5

            Text {
                anchors.centerIn: parent
                text: "+"
                font.pixelSize: 24
            }

            MouseArea {
                anchors.fill: parent
                onClicked: map.zoomLevel += 1
            }
        }

        Rectangle {
            width: 40
            height: 40
            color: "#ffffff"
            border.color: "#cccccc"
            radius: 5

            Text {
                anchors.centerIn: parent
                text: "-"
                font.pixelSize: 24
            }

            MouseArea {
                anchors.fill: parent
                onClicked: map.zoomLevel -= 1
            }
        }
    }

    // Search function
    function searchLocation(searchText) {
        console.log("Searching for: " + searchText);
        // This is a placeholder - actual search will be handled in C++
        return true;
    }

    // Center map function
    function centerMap(lat, lon) {
        var coordinate = QtPositioning.coordinate(lat, lon);
        map.center = coordinate;
        map.zoomLevel = 12;
        marker.coordinate = coordinate;
        marker.visible = true;
        return true;
    }

    Text {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 3
        text: "© OpenStreetMap contributors"
        font.pixelSize: 10
        color: "#333333"
        z: 1
    }

}



