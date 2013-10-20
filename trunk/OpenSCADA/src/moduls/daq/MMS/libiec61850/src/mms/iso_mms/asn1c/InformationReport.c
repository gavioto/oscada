/*
 * Generated by asn1c-0.9.21 (http://lionet.info/asn1c)
 * From ASN.1 module "MMS"
 * 	found in "../mms-extended.asn"
 * 	`asn1c -fskeletons-copy`
 */

#include <asn_internal.h>

#include "InformationReport.h"

static asn_TYPE_member_t asn_MBR_listOfAccessResult_3[] = {
	{ ATF_POINTER, 0, 0,
		-1 /* Ambiguous tag (CHOICE?) */,
		0,
		&asn_DEF_AccessResult,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		""
		},
};
static ber_tlv_tag_t asn_DEF_listOfAccessResult_tags_3[] = {
	(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_SET_OF_specifics_t asn_SPC_listOfAccessResult_specs_3 = {
	sizeof(struct InformationReport__listOfAccessResult),
	offsetof(struct InformationReport__listOfAccessResult, _asn_ctx),
	2,	/* XER encoding is XMLValueList */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_listOfAccessResult_3 = {
	"listOfAccessResult",
	"listOfAccessResult",
	SEQUENCE_OF_free,
	SEQUENCE_OF_print,
	SEQUENCE_OF_constraint,
	SEQUENCE_OF_decode_ber,
	SEQUENCE_OF_encode_der,
	SEQUENCE_OF_decode_xer,
	SEQUENCE_OF_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_listOfAccessResult_tags_3,
	sizeof(asn_DEF_listOfAccessResult_tags_3)
		/sizeof(asn_DEF_listOfAccessResult_tags_3[0]) - 1, /* 1 */
	asn_DEF_listOfAccessResult_tags_3,	/* Same as above */
	sizeof(asn_DEF_listOfAccessResult_tags_3)
		/sizeof(asn_DEF_listOfAccessResult_tags_3[0]), /* 2 */
	0,	/* No PER visible constraints */
	asn_MBR_listOfAccessResult_3,
	1,	/* Single element */
	&asn_SPC_listOfAccessResult_specs_3	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_InformationReport_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct InformationReport, variableAccessSpecification),
		-1 /* Ambiguous tag (CHOICE?) */,
		0,
		&asn_DEF_VariableAccessSpecification,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"variableAccessSpecification"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct InformationReport, listOfAccessResult),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_listOfAccessResult_3,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"listOfAccessResult"
		},
};
static ber_tlv_tag_t asn_DEF_InformationReport_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_InformationReport_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 1 }, /* listOfVariable at 760 */
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 1, -1, 0 }, /* listOfAccessResult at 625 */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 0, 0, 0 } /* variableListName at 762 */
};
static asn_SEQUENCE_specifics_t asn_SPC_InformationReport_specs_1 = {
	sizeof(struct InformationReport),
	offsetof(struct InformationReport, _asn_ctx),
	asn_MAP_InformationReport_tag2el_1,
	3,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_InformationReport = {
	"InformationReport",
	"InformationReport",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No PER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_InformationReport_tags_1,
	sizeof(asn_DEF_InformationReport_tags_1)
		/sizeof(asn_DEF_InformationReport_tags_1[0]), /* 1 */
	asn_DEF_InformationReport_tags_1,	/* Same as above */
	sizeof(asn_DEF_InformationReport_tags_1)
		/sizeof(asn_DEF_InformationReport_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_InformationReport_1,
	2,	/* Elements count */
	&asn_SPC_InformationReport_specs_1	/* Additional specs */
};

