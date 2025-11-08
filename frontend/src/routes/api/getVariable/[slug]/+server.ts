import { redis } from 'bun';

export async function GET({ params }) {
	if(!await redis.exists(`variable:${params.slug}:type`) || !await redis.exists(`variable:${params.slug}:value`)) {
		return new Response(`Variable ${params.slug} was not found!`, { status: 404 })
	}
	

	const type = await redis.get(`variable:${params.slug}:type`);
	let value: string | number | boolean = await redis.get(`variable:${params.slug}:value`) ?? '';

	switch(type) {
	case 'string':
		break;
	case 'int':
		value = parseInt(value);
		break;
	case 'float':
		value = parseFloat(value);
		break;
	case 'bool':
		value = value == '1';
		break;
	}

	return new Response(JSON.stringify({ type, value }), {
		headers: {
			'Content-Type': 'application/json'
		}
	});
}