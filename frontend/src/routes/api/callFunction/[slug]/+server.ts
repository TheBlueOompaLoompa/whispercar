import { redis } from 'bun';

const timeout = 60 * 60 * 2; // 2 minutes until function call explodes

export async function POST({ params, request }) {
	const arg = await request.text();
	await redis.sadd('fcalls', `${params.slug}/${arg}`);

	return new Response(`Added ${params.slug}/${arg} to queue`, {
		headers: {
			'Content-Type': 'text/plain'
		}
	});
}