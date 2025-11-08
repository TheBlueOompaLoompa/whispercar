import { redis } from 'bun';

const timeout = 60 * 60 * 60 * 24 * 7; // One week until variable explodes

export async function POST({ params, request, url }) {
    const value = await request.text();
    redis.set(`variable:${params.slug}:type`, url.searchParams.get('type') ?? 'string');
	redis.set(`variable:${params.slug}:value`, value);

	await redis.expire(`variable:${params.slug}:type`, timeout);
	await redis.expire(`variable:${params.slug}:value`, timeout);

	return new Response(`Set ${params.slug} to ${value}`);
}
