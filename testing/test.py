#!/usr/bin/python3

def validate_origin(expected, actual):
	if expected != actual:
		raise Exception("expecting origin {expected:02X}; found {actual:02X}")
def validate_payload(expected, actual):
	if actual != expected:
		if len(actual) != len(expected):
			diff = len(expected) - len(actual)
			raise Exception(f"expected {len(expected)} byte payload; found {len(actual)} byte payload)")
		for i in range(len(actual)):
			if actual[i] != expected[i]:
				raise Exception(f"Payload differs at byte {i}. Expecting {expected[i]:02X}; found {actual[i]:02X.}")
		raise Exception("payload differs from expected payload (also, internal error)")

def main():
	import sys
	import toml
	import struct
	from base64 import b64decode

	with open(sys.argv[1], 'r') as f:
		expected = toml.load(f)
	with open(sys.argv[2], 'rb') as f:
		obj = f.read()

	# is LC-3 object file
	try:
		(magic, versionMajor, versionMinor) = struct.unpack_from('!6sBB', obj)
		if magic != b"LC3OBJ":
			raise Exception()
	except:
		print("not a valid LC-3 object file")
		sys.exit(1)

	# is supported version
	if versionMajor > 0:
		print(f"major version {versionMajor} not supported")
		sys.exit(2)

	try:
		if versionMajor >= 0:
			validate_v0(expected, obj)
	except:
		print("error while validating", file=sys.stderr)
		sys.exit(3)

	print("success!", file=sys.stderr)

def validate_v0():
	# extract payload
	(payloadOrigin, payloadOffset, payloadSize) = struct.unpack_from('!HIH', obj, 8)
	payload = obj[payloadOffset:payloadOffset+(payloadSize*2)]

	if versionMinor >= 1:
		(labelOffset, labelSize, linkingOffset, linkingSize) = struct.unpack_from('!IIII', obj, 16)
		labels = obj[labelOffset:labelOffset+labelSize]
		linking = obj[linkingOffset:linkingOffset+linkingSize]

	if payloadOrigin != expected['Origin']:
		raise Exception(f"expecting origin {expected.Origin:04X}; found {payloadOrigin:04X}")
	print("origin matches!", file=sys.stderr)

	expectedPayload = b64decode(expected['Payload'])
	if payload != expectedPayload:
		if len(payload) != len(expectedPayload):
			diff = len(expectedPayload) - len(payload)
			raise Exception(f"expected {len(expectedPayload)} byte payload; found {len(payload)} byte payload)")
		for i in range(len(payload)):
			if payload[i] != expectedPayload[i]:
				print("expecting:" + expectedPayload.hex(' ', 2))
				print("found    :" + payload.hex(' ', 2))
				print("error    :" + (" " * (i * 2 + i // 2)) + "^")
				raise Exception(f"payload differs at byte {i}")
		raise Exception("payload differs from expected payload (also, internal error)")
	print("payload matches!", file=sys.stderr)

	if labels or linking or 'Symbols' in expected or 'Linking' in expected:
		print("verifying symbols and linking not yet supported", file=sys.stderr)

if __name__ == "__main__":
	main()

