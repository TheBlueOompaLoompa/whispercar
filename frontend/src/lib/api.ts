import type { LngLat } from "maplibre-gl";

export async function getVariable(name: string) {
    const res = await fetch(`/api/getVariable/${name}`);
    return (await res.json()).value;
}

export async function collectVariables(variables: string) {
    const res = await fetch(`/api/collectVariables?variables=${variables}`);
    return await res.json();
}

export async function callFunction(name: string, arg: string) {
    const res = await fetch(`/api/callFunction/${name}`, {
        method: "POST",
        body: arg
    });
    return await res.json();
}

export async function route(start: LngLat | null = null, end: LngLat | null = null) {
    if(!start || !end) {
        const res = await fetch(`/api/route`, { method: 'GET' });
        return await res.json();
    }else {
        const res = await fetch(`/api/route?start=${start.lng},${start.lat}&end=${end.lng},${end.lat}`, { method: 'POST' });
        return await res.json();
    }
}
