import { redis } from 'bun';

export async function POST({ url }) {
    const res = await fetch(`https://api.openrouteservice.org/v2/directions/foot-walking?api_key=${process.env.OPENROUTEKEY}&start=${url.searchParams.get('start')}&end=${url.searchParams.get('end')}`);
	const text = await res.text();
    await redis.set('nextRoute', text);
    return new Response(text, {
        headers: {
			'Content-Type': 'application/json'
		}
    });
}

export async function GET() {
	return new Response(await redis.get('nextRoute'), {
        headers: {
			'Content-Type': 'application/json'
		}
    });
}