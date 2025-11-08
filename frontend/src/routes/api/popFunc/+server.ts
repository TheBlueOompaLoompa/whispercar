import { redis } from 'bun';

export async function GET({}) {
	return new Response(JSON.stringify(await redis.spop('fcalls')));
}