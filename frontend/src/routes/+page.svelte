<script lang="ts">
    import { getVariable, callFunction, route, collectVariables } from "$lib/api";
    import { GeoJSONSource, LngLat, Map, Marker, Popup } from 'maplibre-gl';
    import 'maplibre-gl/dist/maplibre-gl.css';

    let data = $state({
        valid: false,
        location: new LngLat(0, 0),
        heading: 0
    });

    let lastUpdate = 0;

    let updateLocationInterval = setInterval(async () => {
        if(lastUpdate - 1000 <= Date.now()) {
            lastUpdate = Date.now();

            const res = await collectVariables('heading,pos_valid,longitude,latitude');
            const { heading, pos_valid, longitude, latitude } = res;

            data = {
                valid: pos_valid,
                location: new LngLat(longitude, latitude),
                heading
            };
        }else {
            clearInterval(updateLocationInterval);
            return;
        }
    }, 1000);

    let mapContainer: HTMLDivElement | null = $state(null);
    let setup = false;
    let map: Map | null = $state(null);
    let marker: Marker | null = $state(null);
    let endMarker: Marker | null = $state(null);
    let driving = false;

    $effect(() => {
        if(mapContainer && !setup) {
            setup = true;
            map = new Map({
                style: 'https://tiles.openfreemap.org/styles/liberty',
                center: [-93.230014333, 44.971211667],
                zoom: 18,
                container: 'map',
            });

            map.on('load', async () => {
                if(!map) { console.log('Genuinely how'); return }
                map.addSource('route', {
                    'type': 'geojson',
                    'data': {
                        'type': 'Feature',
                        'properties': {},
                        'geometry': {
                            'type': 'LineString',
                            'coordinates': [
                                [-122.48369693756104, 37.83381888486939],
                                [-122.48348236083984, 37.83317489144141],
                                [-122.48339653015138, 37.83270036637107],
                                [-122.48356819152832, 37.832056363179625],
                                [-122.48404026031496, 37.83114119107971],
                                [-122.48404026031496, 37.83049717427869],
                                [-122.48348236083984, 37.829920943955045],
                                [-122.48356819152832, 37.82954808664175],
                                [-122.48507022857666, 37.82944639795659],
                                [-122.48610019683838, 37.82880236636284],
                                [-122.48695850372314, 37.82931081282506],
                                [-122.48700141906738, 37.83080223556934],
                                [-122.48751640319824, 37.83168351665737],
                                [-122.48803138732912, 37.832158048267786],
                                [-122.48888969421387, 37.83297152392784],
                                [-122.48987674713133, 37.83263257682617],
                                [-122.49043464660643, 37.832937629287755],
                                [-122.49125003814696, 37.832429207817725],
                                [-122.49163627624512, 37.832564787218985],
                                [-122.49223709106445, 37.83337825839438],
                                [-122.49378204345702, 37.83368330777276]
                            ]
                        }
                    }
                });
                map.addLayer({
                    'id': 'route',
                    'type': 'line',
                    'source': 'route',
                    'layout': {
                        'line-join': 'round',
                        'line-cap': 'round'
                    },
                    'paint': {
                        'line-color': '#00afaf',
                        'line-width': 4
                    }
                });

                const res = await route();
                const feature: GeoJSON.GeoJSON<GeoJSON.Geometry, {[name: string]: any; }> = res['features'][0];
                const source = map.getSource('route');
                if(!source) return;
                console.log(source);
                if(source instanceof GeoJSONSource) {
                    source.setData(feature);
                }
            });

            map.on("contextmenu", (ev) => {
                endMarker?.setLngLat(ev.lngLat);
            });
            
            endMarker = new Marker({ color: '#ff0000' })
                .setLngLat([30.5, 50.5])
                .addTo(map);
            
            marker = new Marker({ color: '#00d000' })
                .setLngLat([30.5, 50.5])
                .addTo(map);

            marker.setPopup(markerPopup);
        }
    })

    $effect(() => {
        if(!endMarker) return;
        endMarker.setDraggable(!driving);
    });


    const markerPopup = new Popup();

    $effect(() => {
        if(map && marker) {
            marker.setLngLat(data.location);
            markerPopup.setHTML(`<span>${data.location.lng} ${data.location.lat}</span>`)
            //marker.setRotation(data.heading);
        }
    })

    async function newRoute() {
        if(!map) return;
        if(!marker?.getLngLat() || !endMarker?.getLngLat()) return
        const res = await route(marker.getLngLat(), endMarker.getLngLat());
        const feature: GeoJSON.GeoJSON<GeoJSON.Geometry, {[name: string]: any; }> = res['features'][0];
        const source = map.getSource('route');
        if(!source) return;
        console.log(source);
        if(source instanceof GeoJSONSource) {
            source.setData(feature);
        }
    }
</script>

<main>
    <div id="map" style="height: 100%;" bind:this={mapContainer}></div>
    <bar>
        <button onclick={() => newRoute()}>Route</button>
        <button onclick={() => callFunction('calibrateIMU', '')}>Calibrate IMU</button>
    </bar>
</main>

<style>
    main {
        position: absolute;
        inset: 0px;

        display: flex;
        flex-direction: column;
    }

    bar {
        margin: .5rem;

        display: flex;
        flex-direction: row;
    }
</style>