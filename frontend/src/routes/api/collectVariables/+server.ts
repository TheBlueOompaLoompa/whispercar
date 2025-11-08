import { redis } from 'bun';

async function getVariable(variable: string) {
	if(!await redis.exists(`variable:${variable}:type`) || !await redis.exists(`variable:${variable}:value`)) {
		return variable;
	}
	

	const type = await redis.get(`variable:${variable}:type`);
	let value: string | number | boolean = await redis.get(`variable:${variable}:value`) ?? '';

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

	return { name: variable, type, value };
}

interface Output {
   [key: string]: string | number | boolean;
}

export async function GET({ url }) {
	const variables = `${url.searchParams.get('variables')}`.split(',');
	const res = await Promise.all(variables.map(v => getVariable(v)));
	
	let output: Output = {};

	res.forEach(v => {
		if(typeof v == 'string') return new Response(`Variable ${v} not found`, { status: 404 });
		output[v.name] = v.value;
	});

	return new Response(JSON.stringify(output), {
		headers: {
			'Content-Type': 'application/json'
		}
	});
}