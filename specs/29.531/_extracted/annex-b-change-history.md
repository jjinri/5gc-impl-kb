---
source_spec: "29.531"
source_path: specs/29.531/29531-j60.docx
source_mtime: 1778311321.968539
section: "Annex B"
title: "Annex B (informative): Change history"
generator: spec-split.py
generator_version: 1
chars: 125789
---

# Annex B (informative): Change history

Annex B (informative):
Change history
3GPP TS 29.531 V19.6.0 (2026-03) | 3GPP TS 29.531 V19.6.0 (2026-03)
Technical Specification | Technical Specification
3rd Generation Partnership Project;
Technical Specification Group Core Network and Terminals;
5G System;
Network Slice Selection Services;
Stage 3
(Release 19) | 3rd Generation Partnership Project;
Technical Specification Group Core Network and Terminals;
5G System;
Network Slice Selection Services;
Stage 3
(Release 19)
 | 
 |  | 
 | 
The present document has been developed within the 3rd Generation Partnership Project (3GPP TM) and may be further elaborated for the purposes of 3GPP.
The present document has not been subject to any approval process by the 3GPP Organizational Partners and shall not be implemented.
This Specification is provided for future development work within 3GPP only. The Organizational Partners accept no liability for any use of this Specification.
Specifications and Reports for implementation of the 3GPP TM system should be obtained via the 3GPP Organizational Partners' Publications Offices. | The present document has been developed within the 3rd Generation Partnership Project (3GPP TM) and may be further elaborated for the purposes of 3GPP.
The present document has not been subject to any approval process by the 3GPP Organizational Partners and shall not be implemented.
This Specification is provided for future development work within 3GPP only. The Organizational Partners accept no liability for any use of this Specification.
Specifications and Reports for implementation of the 3GPP TM system should be obtained via the 3GPP Organizational Partners' Publications Offices.
3GPP
Postal address

3GPP support office address
650 Route des Lucioles - Sophia Antipolis
Valbonne - FRANCE
Tel.: +33 4 92 94 42 00 Fax: +33 4 93 65 47 16
Internet
http://www.3gpp.org
Copyright Notification
No part may be reproduced except as authorized by written permission.
The copyright and the foregoing restriction extend to reproduction in all media.

© 2026, 3GPP Organizational Partners (ARIB, ATIS, CCSA, ETSI, TSDSI, TTA, TTC).
All rights reserved.

UMTS™ is a Trade Mark of ETSI registered for the benefit of its members
3GPP™ is a Trade Mark of ETSI registered for the benefit of its Members and of the 3GPP Organizational Partners
LTE™ is a Trade Mark of ETSI registered for the benefit of its Members and of the 3GPP Organizational Partners
GSM® and the GSM logo are registered and owned by the GSM Association
Service Name | Description | Example Consumer
Nnssf_NSSelection | This service enables Network Slice selection in both the Serving PLMN and the HPLMN.

This service also enables Network Slice selection in the hosting operator's network for Indirect Network Sharing deployments. | AMF, V-NSSF, SMF, NWDAF
Nnssf_NSSAIAvailability | This service enables to update the S-NSSAI(s) the NF service consumer (e.g AMF) supports on a per TA basis on the NSSF and to subscribe and notify any change in status, on a per TA basis, of the SNSSAIs available per TA (unrestricted) and the restricted S-NSSAI(s) per PLMN in that TA in the serving PLMN of the UE.

This service also enables the notification of the Network Slice Replacement and Network Slice Instance Replacement to the NF Service Consumer.
This service also enables to subscribe and unsubscribe to the notification of any changes in the status of the NSSAI validity time information. | AMF, V-NSSF
Service Name | Clause | Description | OpenAPI Specification File | apiName | Annex
Nnssf_NSSelection | 6.1 | NSSF Network Slice Selection Service | TS29531_Nnssf_NSSelection.yaml | nnssf-nsselection | A.2
Nnssf_NSSAIAvailability | 6.2 | NSSF NSSAI Availability Service | TS29531_Nnssf_NSSAIAvailability.yaml | nnssf-nssaiavailability | A.3
Resource name | Resource URI | HTTP method or custom operation | Description
Network Slice Information | /network-slice-information | GET | To retrieve network slice information. See clause 6.1.3.2.3.1.

Maps to Nnssf_NSSelection_Get service operation.
Name | Data type | Definition
apiRoot | string | See clause 6.1.1
Name | Data type | P | Cardinality | Description | Applicability
nf-type | NFType | M | 1 | This IE shall contain the NF type of the NF service consumer. | 
nf-id | NfInstanceId | M | 1 | This IE shall contain the NF identifier of the NF service consumer. | 
slice-info-request-for-registration | SliceInfoForRegistration | C | 0..1 | This IE shall be present when the network slice information is requested during the Registration procedure or during EPS to 5GS handover procedure using N26 interface towards an NSSF in the serving PLMN. | 
slice-info-request-for-pdu-session | SliceInfoForPDUSession | C | 0..1 | This IE shall be present when the network slice information is requested during the PDU session establishment procedure. | 
slice-info-request-for-ue-cu | SliceInfoForUEConfigurationUpdate | C | 0..1 | This IE shall be present when the network slice information is requested during UE configuration update procedure. | 
slice-info-request-for-pdn-connection | array(Snssai) | C | 1..N | This IE shall be present when the network slice information is requested during the PDN connection establishment procedure in EPC.
When present, this IE shall include the list of subscribed S-NSSAIs. | RSIPCE
slice-info-request-for-other-purpose | array(Snssai) | C | 1..N | This IE shall be present when the network slice information is requested. | SIOP
home-plmn-id | PlmnId | C | 0..1 | This IE shall be present in the request towards an NSSF in the serving PLMN if the subscriber is a roamer to the serving PLMN or in the Indirect Network Sharing case to indicate the PLMN ID of the SUPI. When present, this IE shall contain the home PLMN Id of the UE. | 
tai | Tai | C | 0..1 | This IE shall be present in the request towards an NSSF in the serving PLMN. When present, this IE shall contain the TAI the UE is currently located. | 
supported-features | SupportedFeatures | C | 0..1 | This IE shall be present if at least one feature defined in clause 6.1.8 is supported. | 
Data type | P | Cardinality | Description
n/a |  |  | 
Data type | P | Cardinality | Response
codes | Description
AuthorizedNetworkSliceInfo | M | 1 | 200 OK | This case represents a successful return of the authorized network slice information selected for the corresponding request.
RedirectResponse | O | 0..1 | 307 Temporary Redirect | Temporary redirection.
(NOTE 2)
RedirectResponse | O | 0..1 | 308 Permanent Redirect | Permanent redirection.
(NOTE 2)
ProblemDetails | O | 0..1 | 403 Forbidden | This represents the case, where the NF service consumer is not authorized to retrieve the slice selection information or the all of the SNSSAIs included in the requested slice selection information is not supported in the PLMN.
The application specific error information shall be provided in the "cause" attribute. The "cause" attribute shall be set to:
-	SNSSAI_NOT_SUPPORTED, if the SNSSAI included in the requested slice selection information is not allowed and there is no default NSSAI value provided in the request.
-	NOT_AUTHORIZED, if the NF service consumer identified by the NF Id is not authorized to retrieve the slice selection information.
See table 6.1.7.3-1 for the description of this error.
NOTE 1:	The mandatory HTTP error status codes for the GET method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP or SEPP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the GET method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP or SEPP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the GET method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP or SEPP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the GET method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP or SEPP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the GET method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP or SEPP, see clause 6.10.9.1 of 3GPP TS 29.500 [4].
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP or SEPP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP or SEPP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Data type | Clause defined | Description
AuthorizedNetworkSliceInfo | 6.1.6.2.2 | Contains the authorized network slice information.
SubscribedSnssai | 6.1.6.2.3 | Contains the subscribed S-NSSAI.
AllowedSnssai | 6.1.6.2.5 | Contains the authorized S-NSSAI and optional mapped home S-NSSAI and network slice instance information.
AllowedNssai | 6.1.6.2.6 | Contains an array of allowed S-NSSAI that constitute the Allowed NSSAI information for the authorized network slice information.
NsiInformation | 6.1.6.2.7 | Contains the API URIs of NRF services to be used to discover NFs/services, subscribe to NF status changes and/or request access tokens within the selected Network Slice instance and optional the Identifier of the selected Network Slice instance.
MappingOfSnssai | 6.1.6.2.8 | Contains the mapping of S-NSSAI in the serving network and the value of the home network.
SliceInfoForRegistration | 6.1.6.2.10 | Contains the slice information requested during a Registration procedure.
SliceInfoForPDUSession | 6.1.6.2.11 | Contains the slice information requested during PDU Session establishment procedure.
ConfiguredSnssai | 6.1.6.2.12 | Contains the configured S-NSSAI authorized by the NSSF in the serving PLMN and optional mapped home S-NSSAI.
SliceInfoForUEConfigurationUpdate | 6.1.6.2.13 | Contains the slice information requested during UE configuration update procedure.
NsagInfo | 6.1.6.2.14 | Contains NSAG information.
SnssaiInfo | 6.1.6.2.15 | Contains the slice information in the response from NSSF
NsiId | 6.1.6.3.2 | Contains the Identifier of the selected Network Slice instance.
RoamingIndication | 6.1.6.3.3 | Contains the indication on roaming.
Data type | Data type | Reference | Comments | Comments
SupportedFeatures | SupportedFeatures | 3GPP TS 29.571 [7] | Used to negotiate the applicability of the features defined in table 6.1.8-1. | Used to negotiate the applicability of the features defined in table 6.1.8-1.
AccessType | AccessType | 3GPP TS 29.571 [7] | Used to specify the access type for which a slice information is applicable. | Used to specify the access type for which a slice information is applicable.
NfServiceSetId | NfServiceSetId | 3GPP TS 29.571 [7] | NF Service Set Identifier | NF Service Set Identifier
RedirectResponse | RedirectResponse | 3GPP TS 29.571 [7] |  | 
NFType | 3GPP TS 29.510 [13] | 3GPP TS 29.510 [13] | 3GPP TS 29.510 [13] | Type of Network Function.
NsSrg | 3GPP TS 29.571 [7] | 3GPP TS 29.571 [7] | 3GPP TS 29.571 [7] | Network slice simultaneous registration groups
NsagId | 3GPP TS 29.571 [7] | 3GPP TS 29.571 [7] | 3GPP TS 29.571 [7] | Network Slice AS Group ID
TaiRange | 3GPP TS 29.510 [13] | 3GPP TS 29.510 [13] | 3GPP TS 29.510 [13] | Range of TAIs
Tai | 3GPP TS 29.571 [7] | 3GPP TS 29.571 [7] | 3GPP TS 29.571 [7] | 
Snssai | 3GPP TS 29.571 [7] | 3GPP TS 29.571 [7] | 3GPP TS 29.571 [7] | 
NfInstanceId | 3GPP TS 29.571 [7] | 3GPP TS 29.571 [7] | 3GPP TS 29.571 [7] | 
Attribute name | Data type | P | Cardinality | Description | Applicability
allowedNssaiList | array(AllowedNssai) | C | 1..N | This IE shall be included if:
-	the NSSF received the Requested NSSAI and the subscribed S-NSSAI(s); or
-	the "requestMapping" flag in the corresponding request was set to "true"; or
-	if neither Requested NSSAI nor the mapping of Requested NSSAI was provided to the NSSF or none of the S-NSSAIs in the Requested NSSAI are permitted (provided that there is at least a subscribed S-NSSAI marked as default which is available in the current TA).

When present, this IE shall contain:
-	the allowed S-NSSAI(s) authorized by the NSSF in the serving PLMN per access type if the Requested NSSAI and the subscribed S-NSSAI(s) received (NOTE 4), or
-	the mapping of S-NSSAI(s) of the VPLMN to corresponding HPLMN S-NSSAI(s) if the requestMapping flag is set to "true", or
-	the mapping of VPLMN S-NSSAIs in the hosting operator’s network to corresponding HPLMN S-NSSAI(s) of the participating operator's network if the requestMapping flag is set to "true" (see clause 5.18.5 of 3GPP TS 23.501 [2]).

NSSF may consider load level information of a Network Slice instance, provided by the NWDAF, to exclude slices that are overloaded.
(NOTE 5, NOTE 6) | 
configuredNssai | array(ConfiguredSnssai) | C | 1..N | This IE shall be included if:

-	the NSSF did not receive any Requested NSSAI; or
-	the Requested NSSAI includes an S-NSSAI that is not valid in the Serving PLMN; or
-	the NSSF has received "defaultConfiguredSnssaiInd" set to "true"; or
-	the network slice information is requested during UE configuration update procedure.

When present, this IE shall contain the configured S-NSSAI(s) authorized by the NSSF in the serving PLMN. See clause 5.15.4.2 of 3GPP TS 23.501 [2].

NSSF may consider load level information of a Network Slice instance, provided by the NWDAF, to exclude slices that are overloaded.
(NOTE 5, NOTE 6) | 
targetAmfSet | string | O | 0..1 | This IE may be included by the NSSF based on configuration and if the NSSF received the Requested NSSAI and the subscribed S-NSSAI(s). When present, this IE shall contain the target AMF set which shall be constructed from PLMN-ID (i.e. three decimal digits MCC and two or three decimal digits MNC), AMF Region Id (8 bit), and AMF Set Id (10 bit).

This IE shall not be included if the "requestMapping" IE was included in the request message and was set to "true".
Pattern: '^[0-9]{3}-[0-9]{2,3}-[A-Fa-f0-9]{2}-[0-3][A-Fa-f0-9]{2}$'
(NOTE 1, NOTE 2, NOTE 3, NOTE 5, NOTE 6) | 
candidateAmfList | array(NfInstanceId) | O | 1..N | This IE may be included by the NSSF based on configuration and if the NSSF received the Requested NSSAI and the subscribed S-NSSAI(s). When present, this IE shall contain the list of candidate AMF(s).

This IE shall not be included if the "requestMapping" IE was included in the request message and was set to "true".
(NOTE 2, NOTE 3, NOTE 5, NOTE 6) | 
rejectedNssaiInPlmn | array(Snssai) | O | 1..N | This IE may be included by the NSSF if the NSSF received:
- the Requested NSSAI and the subscribed S-NSSAI(s) ; or
- "sNssaiForMapping" in the Roaming case.

When present, this IE shall contain the rejected NSSAI in the PLMN.

NSSF may consider load level information of a Network Slice instance, provided by the NWDAF, to exclude slices that are overloaded. Such slices may be included in this attribute.
(NOTE 5, NOTE 6) | 
rejectedNssaiInTa | array(Snssai) | O | 1..N | This IE may be included by the NSSF if the NSSF received:
- the Requested NSSAI and the subscribed S-NSSAI(s) ; or
- "sNssaiForMapping" in the Roaming case.

When present, this IE shall contain the rejected NSSAI in the current TA.

NSSF may consider load level information of a Network Slice instance, provided by the NWDAF, to exclude slices that are overloaded. Such slices may be included in this attribute.
(NOTE 5, NOTE 6) | 
nsiInformation | NsiInformation | C | 0..1 | This IE shall be included by the NSSF if the NSSF received the S-NSSAI (i.e. during PDU session establishment procedure in non-roaming or LBO roaming).
This IE shall also be included by the hNSSF and forwarded by the vNSSF if the hNSSF received the S-NSSAI (i.e. during PDU session establishment procedure in HR roaming).

This IE shall not be included if the "requestMapping" IE was included in the request message and was set to "true".
(NOTE 5, NOTE 6) | 
supportedFeatures | SupportedFeatures | C | 0..1 | This IE shall be present if at least one feature defined in clause 6.1.8 is supported. | 
nrfAmfSet | Uri | O | 0..1 | This IE may be included by the NSSF based on configuration and if the target AMF Set is included.
When present, this IE shall contain the API URI of the NRF NFDiscovery Service (see clause 6.2.1 of 3GPP TS 29.510 [13]) to be used to determine the list of candidate AMF(s) from the AMF Set.
(NOTE 2, NOTE 5, NOTE 6) | 
nrfAmfSetNfMgtUri | Uri | O | 0..1 | This IE should be present if the nrfAmfSet is present. When present, it shall contain the API URI of the NRF NFManagement Service (see clause 6.1.1 of 3GPP TS 29.510 [13]).
(NOTE 2, NOTE 5, NOTE 6) | 
nrfAmfSetAccessTokenUri | Uri | O | 0..1 | When present, this IE shall contain the API URI of the NRF Access Token Service (see clause 6.3.2 of 3GPP TS 29.510 [13]).
(NOTE 2, NOTE 5, NOTE 6) | 
nrfOauth2Required | map(boolean) | O | 1..N | This IE may be present if the nrfAmfSet IE or the nrfAmfSetNfMgtUri IE is present.
When present, this IE shall indicate whether the NRF requires Oauth2-based authorization for accessing its services.
The key of the map shall be the name of an NRF service, e.g. "nnrf-nfm" or "nnrf-disc".

The value of each entry of the map shall be encoded as follows:
- true: OAuth2 based authorization is required.
- false: OAuth2 based authorization is not required.
If this IE is present and set to true, then the nrfAmfSetAccessTokenUri IE shall be present and shall be used to request access token for NRF services.

The absence of this IE means that the NRF has not provided any indication about its usage of Oauth2 for authorization.
(NOTE 2, NOTE 5, NOTE 6) | 
targetAmfServiceSet | NfServiceSetId | O | 0..1 | When present, this IE shall contain the target AMF service set.
(NOTE 1, NOTE 2, NOTE 5, NOTE 6) | 
targetNssai | array(Snssai) | O | 1..N | This IE may be included by the NSSF if the NSSF received the Requested NSSAI and TAI, or the NSSF received the rejected NSSAI of current Registration Area.

When present, this IE shall contain S-NSSAI(s) as defined in clause 5.3.4.3.3 of 3GPP TS 23.501 [2].
(NOTE 5, NOTE 6) | TargetNssai
nsagInfos | array(NsagInfo) | C | 1..N | This attribute shall be present if the AMF has indicated the support of NSAG by the UE, and the NSAG information is available to the NSSF.
This attribute contains the list of NSAG information.
(NOTE 5, NOTE 6) | 
mappingOfNssai | array(MappingOfSnssai) | C | 1..N | This IE shall be included by the NSSF if the NSSF receives the query parameter slice-info-request-for-pdn-connection.

When present, this IE shall contain the mapping of S-NSSAI of the VPLMN to corresponding HPLMN S-NSSAI, for the HPLMN S-NSSAIs included in the request. | RSIPCE
snssaiInfoRspData | map(SnssaiInfo) | C | 1..N | This IE shall be included by the NSSF if the NSSF receives the query parameter slice-info-request-for-other-purpose.

When present, this IE contains the map of slice information, where the key of the map is the Snssai. | SIOP
NOTE 1:	The NF Service Consumer uses the PLMN ID, AMF Region, AMF Set and AMF Service Set to perform a NF Discovery to the NRF.
NOTE 2:	These attributes should be absent if the NSSF provides a Target NSSAI in targetNssai attribute in order to redirect or handover the UE to a cell of another TA as defined in clause 5.15.5.2.1 of 3GPP TS 23.501 [2].
NOTE 3:	The targetAmfSet attribute and the candidateAmfList attribute should not be present simultaneously.
NOTE 4: 	The authorized allowed S-NSSAIs may contain also Subscribed S-NSSAIs not marked as default and/or that were not part of the Requested NSSAI based on local policy configuration at NSSF.
NOTE 5:	These attributes should be absent if the mappingOfNssai attribute is present in the message.
NOTE 6:	These attributes should be absent if the snssaiInfoRspData attribute is present in the message. | NOTE 1:	The NF Service Consumer uses the PLMN ID, AMF Region, AMF Set and AMF Service Set to perform a NF Discovery to the NRF.
NOTE 2:	These attributes should be absent if the NSSF provides a Target NSSAI in targetNssai attribute in order to redirect or handover the UE to a cell of another TA as defined in clause 5.15.5.2.1 of 3GPP TS 23.501 [2].
NOTE 3:	The targetAmfSet attribute and the candidateAmfList attribute should not be present simultaneously.
NOTE 4: 	The authorized allowed S-NSSAIs may contain also Subscribed S-NSSAIs not marked as default and/or that were not part of the Requested NSSAI based on local policy configuration at NSSF.
NOTE 5:	These attributes should be absent if the mappingOfNssai attribute is present in the message.
NOTE 6:	These attributes should be absent if the snssaiInfoRspData attribute is present in the message. | NOTE 1:	The NF Service Consumer uses the PLMN ID, AMF Region, AMF Set and AMF Service Set to perform a NF Discovery to the NRF.
NOTE 2:	These attributes should be absent if the NSSF provides a Target NSSAI in targetNssai attribute in order to redirect or handover the UE to a cell of another TA as defined in clause 5.15.5.2.1 of 3GPP TS 23.501 [2].
NOTE 3:	The targetAmfSet attribute and the candidateAmfList attribute should not be present simultaneously.
NOTE 4: 	The authorized allowed S-NSSAIs may contain also Subscribed S-NSSAIs not marked as default and/or that were not part of the Requested NSSAI based on local policy configuration at NSSF.
NOTE 5:	These attributes should be absent if the mappingOfNssai attribute is present in the message.
NOTE 6:	These attributes should be absent if the snssaiInfoRspData attribute is present in the message. | NOTE 1:	The NF Service Consumer uses the PLMN ID, AMF Region, AMF Set and AMF Service Set to perform a NF Discovery to the NRF.
NOTE 2:	These attributes should be absent if the NSSF provides a Target NSSAI in targetNssai attribute in order to redirect or handover the UE to a cell of another TA as defined in clause 5.15.5.2.1 of 3GPP TS 23.501 [2].
NOTE 3:	The targetAmfSet attribute and the candidateAmfList attribute should not be present simultaneously.
NOTE 4: 	The authorized allowed S-NSSAIs may contain also Subscribed S-NSSAIs not marked as default and/or that were not part of the Requested NSSAI based on local policy configuration at NSSF.
NOTE 5:	These attributes should be absent if the mappingOfNssai attribute is present in the message.
NOTE 6:	These attributes should be absent if the snssaiInfoRspData attribute is present in the message. | NOTE 1:	The NF Service Consumer uses the PLMN ID, AMF Region, AMF Set and AMF Service Set to perform a NF Discovery to the NRF.
NOTE 2:	These attributes should be absent if the NSSF provides a Target NSSAI in targetNssai attribute in order to redirect or handover the UE to a cell of another TA as defined in clause 5.15.5.2.1 of 3GPP TS 23.501 [2].
NOTE 3:	The targetAmfSet attribute and the candidateAmfList attribute should not be present simultaneously.
NOTE 4: 	The authorized allowed S-NSSAIs may contain also Subscribed S-NSSAIs not marked as default and/or that were not part of the Requested NSSAI based on local policy configuration at NSSF.
NOTE 5:	These attributes should be absent if the mappingOfNssai attribute is present in the message.
NOTE 6:	These attributes should be absent if the snssaiInfoRspData attribute is present in the message. | NOTE 1:	The NF Service Consumer uses the PLMN ID, AMF Region, AMF Set and AMF Service Set to perform a NF Discovery to the NRF.
NOTE 2:	These attributes should be absent if the NSSF provides a Target NSSAI in targetNssai attribute in order to redirect or handover the UE to a cell of another TA as defined in clause 5.15.5.2.1 of 3GPP TS 23.501 [2].
NOTE 3:	The targetAmfSet attribute and the candidateAmfList attribute should not be present simultaneously.
NOTE 4: 	The authorized allowed S-NSSAIs may contain also Subscribed S-NSSAIs not marked as default and/or that were not part of the Requested NSSAI based on local policy configuration at NSSF.
NOTE 5:	These attributes should be absent if the mappingOfNssai attribute is present in the message.
NOTE 6:	These attributes should be absent if the snssaiInfoRspData attribute is present in the message.
Attribute name | Data type | P | Cardinality | Description
subscribedSnssai | Snssai | M | 1 | This IE shall contain the subscribed S-NSSAI.
defaultIndication | boolean | O | 0..1 | If it is set, the subscribed S-NSSAI is a default subscribed S-NSSAI.
subscribedNsSrgList | array(NsSrg) | O | 1..N | If present, this IE shall contain the subscribed network slice simultaneous registration groups applicable to the subscribedSnssai.
Attribute name | Data type | P | Cardinality | Description
allowedSnssai | Snssai | M | 1 | This IE shall contain the allowed S-NSSAI in the serving PLMN.
nsiInformationList | array(NsiInformation) | O | 1..N | This IE may be present when the NSSF provides the Allowed NSSAI information to the NF service consumer (e.g AMF). If present, this IE shall include the information related to the network slice instance corresponding to the allowed S-NSSAI.
mappedHomeSnssai | Snssai | O | 0..1 | When present, this IE shall contain the mapped S-NSSAI value of home network corresponding to the allowed S-NSSAI in the serving PLMN.
Attribute name | Data type | P | Cardinality | Description
allowedSnssaiList | array(AllowedSnssai) | M | 1..N | This IE shall contain the allowed S-NSSAI in the serving PLMN.
(NOTE)
accessType | AccessType | M | 1 | This IE shall contain the access type to which this Allowed NSSAI belongs.
NOTE:	The maximum number of allowed S-NSSAIs shall not exceed the maximum number defined in 3GPP TS 24.501 [20]. | NOTE:	The maximum number of allowed S-NSSAIs shall not exceed the maximum number defined in 3GPP TS 24.501 [20]. | NOTE:	The maximum number of allowed S-NSSAIs shall not exceed the maximum number defined in 3GPP TS 24.501 [20]. | NOTE:	The maximum number of allowed S-NSSAIs shall not exceed the maximum number defined in 3GPP TS 24.501 [20]. | NOTE:	The maximum number of allowed S-NSSAIs shall not exceed the maximum number defined in 3GPP TS 24.501 [20].
Attribute name | Data type | P | Cardinality | Description
nrfId | Uri | M | 1 | This IE shall contain the API URI of the NRF NFDiscovery Service (see clause 6.2.1 of 3GPP TS 29.510 [13]) to be used to select the NFs/services within the selected Network Slice instance.
nsiId | NsiId | O | 0..1 | This IE may be optionally included by the NSSF. When present, this IE shall contain the Identifier of the selected Network Slice instance
nrfNfMgtUri | Uri | O | 0..1 | This IE should be present. When present, it shall contain the API URI of the NRF NFManagement Service (see clause 6.1.1 of 3GPP TS 29.510 [13]).
nrfAccessTokenUri | Uri | O | 0..1 | When present, this IE shall contain the API URI of the NRF Access Token Service (see clause 6.3.2 of 3GPP TS 29.510 [13]).
nrfOauth2Required | map(boolean) | O | 1..N | This IE may be present if the nrfId IE or the nrfNfMgtUri IE is present.
When present, this IE shall indicate whether the NRF requires Oauth2-based authorization for accessing its services.
The key of the map shall be the name of an NRF service, e.g. "nnrf-nfm" or "nnrf-disc".

The value of each entry of the map shall be encoded as follows:
- true: OAuth2 based authorization is required.
- false: OAuth2 based authorization is not required.
The absence of this IE means that the NRF has not provided any indication about its usage of Oauth2 for authorization.
Attribute name | Data type | P | Cardinality | Description
servingSnssai | Snssai | M | 1 | This IE shall contain the S-NSSAI value of serving network.
homeSnssai | Snssai | M | 1 | This IE shall contain the mapped S-NSSAI value of home network.
Attribute name | Data type | P | Cardinality | Description
subscribedNssai | array(SubscribedSnssai) | C | 1..N | This IE shall be included during the initial registration procedure or during mobility registration procedure in 5GS. This IE may also be included during EPS to 5GS handover procedure/Idle mode Mobility Registration Procedure using N26 interface or the handover procedure within 5GS. When present, this IE shall contain the list of subscribed S-NSSAIs along with an indication for each S-NSSAI if it is a default S-NSSAI.
For inbound roaming case, this IE shall contain the subscribed HPLMN NSSAI of the UE.
allowedNssaiCurrentAccess | AllowedNssai | C | 0..1 | This IE shall be included during an initial registration procedure in 5GS or during mobility registration update procedure in 5GS with a native 5G-GUTI as the old GUTI, and an Allowed NSSAI for the current access type of the UE is available at the NF service consumer (e.g. AMF).
allowedNssaiOtherAccess | AllowedNssai | C | 0..1 | This IE shall be present during an initial registration procedure in 5GS or during mobility registration update procedure in 5GS with a native 5G-GUTI as the old GUTI, and if the UE was registered with the NF service consumer (e.g. AMF) earlier for another access type and an Allowed NSSAI for the other access type is available at the NF service consumer (e.g. AMF).
sNssaiForMapping | array(Snssai) | C | 1..N | This IE shall be included if the requestMapping IE is set to true. When included, this IE shall contain:
-	the set of S-NSSAIs obtained from PGW+SMF in the HPLMN for PDU sessions that are handed over from EPS to 5GS; or
-	the set of HPLMN S-NSSAIs obtained from source AMF during handover procedure within 5GS; or
-	the S-NSSAIs for the HPLMN received from the UE during EPS to 5GS Idle mode Mobility Registration Procedure using N26 interface/idle state mobility registration procedure in 5GS; or
-	the set of HPLMN (participating operator's network) S-NSSAIs during registration procedure in the case of Indirect Network Sharing (see clause 5.18.5 of 3GPP TS 23.501 [2]).
mappingOfNssai | array(MappingOfSnssai) | O | 1..N | This IE may be present when the network slice information is requested during the Registration procedure in roaming scenarios. If present, this IE shall contain the mapping of S-NSSAI of the VPLMN to corresponding HPLMN S-NSSAI, for the S-NSSAIs included in the requestedNssai and allowedNssai IEs for the current and other access types.
This IE may also be present when the network slice information is requested during EPS to 5GS handover procedure using N26 interface or the handover procedure within 5GS. If present, this IE shall contain the mapping of S-NSSAI of the VPLMN to corresponding HPLMN S-NSSAI, for the S-NSSAIs included in the requestedNssai IE.
requestedNssai | array(Snssai) | O | 1..N | This IE may contain the set of S-NSSAIs requested by the UE, it shall be the S-NSSAIs in hPLMN in non-roaming scenario, or the S-NSSAIs in vPLMN in LBO/HR roaming scenario.
During EPS to 5GS handover procedure using N26 interface, this IE may contain the set of S-NSSAIs in the serving PLMN obtained from PGW+SMF in VPLMN, or mapped from the set of S-NSSAIs obtained from PGW+SMF in the HPLMN.
During handover procedure within 5GS, this IE may contain the set of S-NSSAIs in the serving PLMN obtained from the source AMF, or mapped from the set of HPLMN S-NSSAIs obtained from source AMF.
defaultConfiguredSnssaiInd | boolean | C | 0..1 | This IE shall be present when the UE includes the Default Configured NSSAI Indication during the Registration procedure or when the AMF receives the network slice subscription change indication (NSSCI) from UDM.

true: The NSSF is required to return Configured NSSAI;
false (default): The NSSF is not required to return Configured NSSAI.
requestMapping | boolean | O | 0..1 | This IE may be present when the Nnssf_NSSelection_Get procedure is invoked during:
-	EPS to 5GS Mobility Registration Procedure (Idle State) using N26 interface or during EPS to 5GS handover procedure using N26 interface, or
-	Idle state Mobility Registration Procedure or handover procedure in 5GS, or
-	Registration procedure to get the slice mapping information in the case of Indirect Network Sharing as specified in clause 5.18.5 of 3GPP TS 23.501 [2].

When present this IE shall indicate to the NSSF that the NSSF shall return:
-	The VPLMN specific mapped SNSSAI values for the S-NSSAI values in the sNssaiForMapping IE, or
-	The VPLMN (hosting operator's network) mapped S-NSSAI values for the S-NSSAI values in the sNssaiForMapping IE.
ueSupNssrgInd | boolean | C | 0..1 | This IE shall be present in the request towards an NSSF in the serving PLMN when UE has indicated the support of NSSRG feature. When present, this IE shall contain the indication of UE support of subscription-based restrictions to simultaneous registration of network slice feature.
This IE shall be set as follows:
- true: the UE supports NSSRG feature
- false: the UE does not support NSSRG feature.
suppressNssrgInd | boolean | O | 0..1 | When present, this IE shall contain the UDM indication to provide all subscribed S-NSSAIs for UEs not indicating support of subscription-based restrictions to simultaneous registration of network slices. This IE may be present and set to true if the ueSupNssrgInd is set to false.

This IE shall be set as follows:
- true: UDM Indication to suppress NSSRG is present and set to TRUE
- false: UDM Indication to suppress NSSRG is set to FALSE or not present
nsagSupported | boolean | C | 0..1 | This IE shall be present if the UE has indicated support of NSAG in the 5GMM procedure.

true: the UE supports NSAG.
false (default): the UE does not support NSAG.
Attribute name | Data type | P | Cardinality | Description
sNssai | Snssai | M | 1 | This IE shall contain the requested S-NSSAI for the PDU session, when the AMF queries the NSSF in the serving PLMN. When the vNSSF queries the hNSSF during PDU session establishment for home routed roaming case, this IE shall contain the S-NSSAI from the HPLMN that maps to the S-NSSAI from the Allowed NSSAI of the Serving PLMN, as obtained from the NF Service Consumer of the vNSSF.
roamingIndication | RoamingIndication | M | 1 | This IE shall contain the indication whether the UE is in non-roaming, LBO roaming or HR roaming.
homeSnssai | Snssai | C | 0..1 | This IE shall be included by the NF Service Consumer (e.g. AMF) towards the vNSSF during PDU session establishment procedure in home routed roaming scenario. This IE shall contain the S-NSSAI of the HPLMN that maps to the S-NSSAI from the Allowed NSSAI of the Serving PLMN when the UE in the roaming scenario.
Attribute name | Data type | P | Cardinality | Description
configuredSnssai | Snssai | M | 1 | This IE shall contain the configured S-NSSAI in the serving PLMN.
mappedHomeSnssai | Snssai | O | 0..1 | When present, this IE shall contain the mapped S-NSSAI value of home network corresponding to the configured S-NSSAI in the serving PLMN.
Attribute name | Data type | P | Cardinality | Description | Applicability
subscribedNssai | array(SubscribedSnssai) | C | 1..N | This IE shall be included during UE configuration update procedure in 5GS. When present, this IE shall contain the list of subscribed S-NSSAIs along with an indication for each S-NSSAI if it is a default S-NSSAI. | 
allowedNssaiCurrentAccess | AllowedNssai | O | 0..1 | This IE may be included during UE configuration update procedure in 5GS. When present, this IE shall contain the list of allowed S-NSSAIs in the AMF for the current access type of the UE. | 
allowedNssaiOtherAccess | AllowedNssai | O | 0..1 | This IE may be included during UE configuration update procedure in 5GS. When present, this IE shall contain the list of allowed S-NSSAIs in the AMF for the other access type of the UE. | 
defaultConfiguredSnssaiInd | boolean | O | 0..1 | This IE may be present if the UE included the Default Configured NSSAI Indication during the recent Registration procedure, or if the AMF recently received the network slice subscription change indication (NSSCI) from UDM.

true: The NSSF is required to return Configured NSSAI;
false (default): The NSSF is not required to return Configured NSSAI. | 
requestedNssai | array(Snssai) | O | 1..N | This IE may contain the set of S-NSSAIs requested by the UE in the recent registration procedure, it shall be the S-NSSAIs in hPLMN in non-roaming scenario, or the S-NSSAIs in vPLMN in LBO/HR roaming scenario. | 
mappingOfNssai | array(MappingOfSnssai) | O | 1..N | This IE may be present when the network slice information is requested during UE configuration update procedure in roaming scenarios. If present, this IE shall contain the mapping of S-NSSAI of the VPLMN to corresponding HPLMN S-NSSAI, for the S-NSSAIs included in the requestedNssai and the allowedNssai IEs for the current and other access types. | 
ueSupNssrgInd | boolean | C | 0..1 | This IE shall be present in the request towards an NSSF in the serving PLMN when UE has indicated the support of NSSRG feature. When present, this IE shall contain the indication of UE support of subscription-based restrictions to simultaneous registration of network slice feature.
This IE shall be set as follows:
- true: the UE supports NSSRG feature
- false: the UE does not support NSSRG feature. | 
suppressNssrgInd | boolean | O | 0..1 | When present, this IE shall contain the UDM indication to provide all subscribed S-NSSAIs for UEs not indicating support of subscription-based restrictions to simultaneous registration of network slices. This IE may be present and set to true if the ueSupNssrgInd is set to false.

This IE shall be set as follows:
- true: UDM Indication to suppress NSSRG is present and set to TRUE
- false: UDM Indication to suppress NSSRG is set to FALSE or not present | 
rejectedNssaiRa | array(Snssai) | O | 1..N | This IE may be present when the UE is needed to be redirected to the dedicated frequency band(s) for an S-NSSAI (as specified in clause 5.3.4.3.3 of 3GPP TS 23.501 [2]).

When present, this IE shall indicate the rejected S-NSSAI(s) of the Registration Area. | TargetNssai
nsagSupported | boolean | C | 0..1 | This IE shall be present if the UE has indicated support of NSAG in the 5GMM procedure.

true: the UE supports NSAG.
false (default): the UE does not support NSAG. | 
Attribute name | Data type | P | Cardinality | Description
nsagIds | array(NsagId) | M | 1..N | The list of NSAG IDs, see 3GPP TS 38.413 [21]
snssaiList | array(Snssai) | M | 1..N | This attribute contains the S-NSSAI(s) which are associated with the NSAGs identified by the nsagIds.
taiList | array(Tai) | O | 1..N | This attribute indicates the TA(s) within which the association between the NSAGs identified by the nsagIds and the S-NSSAI(s) is valid.
(NOTE)
taiRangeList | array(TaiRange) | O | 1..N | This attribute indicates the TA(s) within which the association between the NSAGs identified by the nsagIds and the S-NSSAI(s) is valid.
(NOTE)
NOTE:	The absence of both taiList and taiRangeList attributes means the association between the NSAGs identified by the nsagIds and the S-NSSAI(s) is valid across the PLMN | NOTE:	The absence of both taiList and taiRangeList attributes means the association between the NSAGs identified by the nsagIds and the S-NSSAI(s) is valid across the PLMN | NOTE:	The absence of both taiList and taiRangeList attributes means the association between the NSAGs identified by the nsagIds and the S-NSSAI(s) is valid across the PLMN | NOTE:	The absence of both taiList and taiRangeList attributes means the association between the NSAGs identified by the nsagIds and the S-NSSAI(s) is valid across the PLMN | NOTE:	The absence of both taiList and taiRangeList attributes means the association between the NSAGs identified by the nsagIds and the S-NSSAI(s) is valid across the PLMN
Attribute name | Data type | P | Cardinality | Description
nsiIds | array(NsiId) | C | 1..N | This IE shall be present if the slice-info-request-for-other-purpose query parameter is sent from NWDAF.

When present, this IE shall contain the Identifiers of the Network Slice instances.
Type Name | Type Definition | Description
NsiId | string | Represents the Network Slice Instance Identifier
Enumeration value | Description
"NON_ROAMING" | This value indicates that the UE is not roaming.
"LOCAL_BREAKOUT" | This value indicates that the UE is roaming but is using a local breakout PDU session.
"HOME_ROUTED_ROAMING" | This value indicates that the UE is roaming and is using a home routed PDU session.
Application Error | HTTP status code | Description
SNSSAI_NOT_SUPPORTED | 403 Forbidden | This cause value shall be set when the requested slice selection information is for SNSSAI(s) not supported.
NOT_AUTHORIZED | 403 Forbidden | The request is rejected due to the NF service consumer is not authorized to retrieve the slice selection information.
Feature Number | Feature | M/O | Description
1 | ES3XX | M | Extended Support of HTTP 307/308 redirection

An NF Service Consumer (e.g. AMF) that supports this feature shall support handling of HTTP 307/308 redirection for any service operation of the Nnssf_NSSelection service. An NF Service Consumer that does not support this feature does only support HTTP redirection as specified for 3GPP Release 15.
2 | TargetNssai | O | Target NSSAI

An NF Service Consumer (e.g. AMF) and NSSF that supports this feature shall support handling of Target NSSAI as specified in clause 5.3.4.3.3 and clause 5.15.5.2.1 of 3GPP TS 23.501 [2].
3 | RSIPCE | O | Retrieval of Slice Information during PDN Connection Establishment

An NF Service Consumer (e.g. SMF+PGW-C) and NSSF that supports this feature shall support slice information retrieval including PDN Connection Establishment as specified in clause 4.11.0a.5 of 3GPP TS 23.502 [3].
4 | SIOP | O | Slice Information for Other Purpose

An NF Service Consumer and NSSF that supports this feature shall support slice information retrieval including:
slice information retrieval by NWDAF during network slice load analytics as specified in clause 6.3.4 of 3GPP TS 23.288 [22].
Feature number: The order number of the feature within the supportedFeatures attribute (starting with 1).
Feature: A short name that can be used to refer to the bit and to the feature.
M/O: Defines if the implementation of the feature is mandatory ("M") or optional ("O").
Description: A clear textual description of the feature. | Feature number: The order number of the feature within the supportedFeatures attribute (starting with 1).
Feature: A short name that can be used to refer to the bit and to the feature.
M/O: Defines if the implementation of the feature is mandatory ("M") or optional ("O").
Description: A clear textual description of the feature. | Feature number: The order number of the feature within the supportedFeatures attribute (starting with 1).
Feature: A short name that can be used to refer to the bit and to the feature.
M/O: Defines if the implementation of the feature is mandatory ("M") or optional ("O").
Description: A clear textual description of the feature. | Feature number: The order number of the feature within the supportedFeatures attribute (starting with 1).
Feature: A short name that can be used to refer to the bit and to the feature.
M/O: Defines if the implementation of the feature is mandatory ("M") or optional ("O").
Description: A clear textual description of the feature.
Resource name | Resource URI | HTTP method or custom operation | Description
NSSAI Availability Store | /nssai-availability | OPTIONS | Discover the communication options supported by the NSSF for this resource.
NSSAI Availability Document | /nssai-availability/{nfId} | PUT | Updates the NSSF with the S-NSSAIs the NF service consumer (e.g. AMF) supports per TA.
NSSAI Availability Document | /nssai-availability/{nfId} | PATCH | Updates the NSSF with the S-NSSAIs the NF service consumer (e.g. AMF) supports per TA.
NSSAI Availability Document | /nssai-availability/{nfId} | DELETE | Delete the resource of the S-NSSAIs supported per TA by the NF service consumer (e.g. AMF)
NSSAI Availability Notification Subscriptions Collection | /nssai-availability/subscriptions | POST | Create a subscription to the notification of any changes to the NSSAI availability information, Network Slice Replacement or Network Slice Instance Replacement.
Individual NSSAI Availability Notification Subscriptions | /nssai-availability/subscriptions/{subscriptionId} | DELETE | Unsubscribe to the notification of any changes to the NSSAI availability information, Network Slice Replacement or Network Slice Instance Replacement.
Individual NSSAI Availability Notification Subscriptions | /nssai-availability/subscriptions/{subscriptionId} | PATCH | Modify a subscription.
Name | Data type | Definition
apiRoot | string | See clause 6.2.1
nfId | NfInstanceId | Represents the Identifier of the AMF for which the NSSAI Availability information is updated.
Data type | P | Cardinality | Description
NssaiAvailabilityInfo | M | 1 | This IE contains the information regarding the NssaiAvailabilityData for the NF Service Consumer (e.g AMF).
Data type | P | Cardinality | Response
codes | Description
AuthorizedNssaiAvailabilityInfo | M | 1 | 200 OK | This case represents a successful update of the NSSF with the S-NSSAIs the AMF supports per TA.
The authorized NSSAI availability (i.e. S-NSSAIs available per TA (unrestricted) and any S-NSSAIs restricted per PLMN in that TA in the serving PLMN of the UE) information shall be returned in the response content.
N/A |  |  | 204 No Content | This case represents a successful update of the NSSF with the S-NSSAIs the AMF supports per TA, and the authorized NSSAI availability is empty after the update.
RedirectResponse | O | 0..1 | 307 Temporary Redirect | Temporary redirection.
(NOTE 2)
RedirectResponse | O | 0..1 | 308 Permanent Redirect | Permanent redirection.
(NOTE 2)
ProblemDetails | O | 0..1 | 403 Forbidden | When the NF service consumer is not authorized to update the NSSAI availability information or the TAI/S-NSSAI information provided is not supported in the PLMN, the "cause" attribute shall be set to:
-	SNSSAI_NOT_SUPPORTED, if the S-NSSAI provided is not supported in the PLMN.
-	NOT_AUTHORIZED, if the NF service consumer identified by the NF Id is not authorized to update the NSSAI availability information.
See table 6.2.7.3-1 for the description of this error.
NOTE 1:	The mandatory HTTP error status codes for the PUT method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the PUT method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the PUT method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the PUT method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the PUT method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4].
Name | Data type | P | Cardinality | Description
Content-Encoding | string | O | 0..1 | Content-Encoding, described in IETF RFC 9110 [18]
Name | Data type | P | Cardinality | Description
Accept-Encoding | string | O | 0..1 | Accept-Encoding, described in IETF RFC 9110 [18]
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Data type | P | Cardinality | Description
PatchDocument | M | 1 | This IE contains the information regarding the JSON patch instructions for updating the supportedSnssai(s) in NssaiAvailabilityInfo.
Data type | P | Cardinality | Response
codes | Description
AuthorizedNssaiAvailabilityInfo | M | 1 | 200 OK | This case represents a successful update of the NSSF with the S-NSSAIs the AMF supports per TA.
If the authorized NSSAI availability (i.e. S-NSSAIs available per TA (unrestricted) and any S-NSSAIs restricted per PLMN in that TA in the serving PLMN of the UE) is changed, the NSSF shall return a data structure of type "AuthorizedNssaiAvailabilityInfo" in the response payload body.
N/A |  |  | 204 No Content | This case represents a successful update of the NSSF with the S-NSSAIs the AMF supports per TA, and the authorized NSSAI availability is empty after the update.
RedirectResponse | O | 0..1 | 307 Temporary Redirect | Temporary redirection.
(NOTE 2)
RedirectResponse | O | 0..1 | 308 Permanent Redirect | Permanent redirection.
(NOTE 2)
ProblemDetails | O | 0..1 | 403 Forbidden | When the NF service consumer is not authorized to update the NSSAI availability information or the S-NSSAI information provided is not supported in the PLMN, the "cause" attribute shall be set to:
-	SNSSAI_NOT_SUPPORTED, if the S-NSSAI provided is not supported in the PLMN.
-	NOT_AUTHORIZED, if the NF service consumer identified by the NF Id is not authorized to update the NSSAI availability information.
See table 6.2.7.3-1 for the description of this error.
ProblemDetails | O | 0..1 | 404 Not Found | The "cause" attribute may be used to indicate one of the following application error:
-	RESOURCE_NOT_FOUND
See table 6.2.7.3-1 for the description of this error.
NOTE 1:	The mandatory HTTP error status codes for the PATCH method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the PATCH method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the PATCH method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the PATCH method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the PATCH method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4].
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Data type | P | Cardinality | Description
n/a |  |  | 
Data type | P | Cardinality | Response
codes | Description
n/a |  |  | 204 No Content | 
RedirectResponse | O | 0..1 | 307 Temporary Redirect | Temporary redirection.
(NOTE 2)
RedirectResponse | O | 0..1 | 308 Permanent Redirect | Permanent redirection.
(NOTE 2)
ProblemDetails | O | 0..1 | 404 Not Found | The "cause" attribute may be used to indicate one of the following application error:
-	RESOURCE_NOT_FOUND
See table 6.2.7.3-1 for the description of this error.
NOTE 1:	The mandatory HTTP error status codes for the DELETE method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the DELETE method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the DELETE method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the DELETE method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the DELETE method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4].
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Name | Data type | Definition
apiRoot | string | See clause 6.2.1
Data type | P | Cardinality | Description
NssfEventSubscriptionCreateData | M | 1 | This IE contains the information of an NSSF Event Subscription to be created.
Data type | P | Cardinality | Response
codes | Description
NssfEventSubscriptionCreatedData | M | 1 | 201 Created | This case represents a successful creation of an NSSF Event subscription.
RedirectResponse | O | 0..1 | 307 Temporary Redirect | Temporary redirection. The response shall include a Location header field containing a different URI, or the same URI if this is a redirection triggered by an SCP to the same target resource via another SCP. In the former case, the URI shall be an alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
(NOTE 2)
RedirectResponse | O | 0..1 | 308 Permanent Redirect | Permanent redirection. The response shall include a Location header field containing a different URI, or the same URI if this is a redirection triggered by an SCP to the same target resource via another SCP. In the former case, the URI shall be an alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
(NOTE 2)
ProblemDetails | O | 0..1 | 403 Forbidden | The "cause" attribute may be used to indicate one of the following application error:
-	NOT_AUTHORIZED
See table 6.2.7.3-1 for the description of these errors.
ProblemDetails | O | 0..1 | 501 Not Implemented | The "cause" attribute may be used to indicate one of the following application errors:
-	UNSUPPORTED_EVENT_TYPE
NOTE 1:	The mandatory HTTP error status codes for the POST method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the POST method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the POST method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the POST method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the POST method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4].
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | Contains the URI of the newly created resource, according to the structure: {apiRoot}/nnssf-nssaiavailability/<apiVersion>/nssai-availability/subscriptions/{subscriptionId}
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
Or the same URI, if a request is redirected to the same target resource via a different SCP.
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
Or the same URI, if a request is redirected to the same target resource via a different SCP.
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Name | Data type | Definition
apiRoot | string | See clause 6.2.1
subscriptionId | string | Represents the Identifier of the subscription.
Data type | P | Cardinality | Description
N/A |  |  | 
Data type | P | Cardinality | Response
codes | Description
N/A |  |  | 204 No Content | This case represents a successful deletion of the subscription.
RedirectResponse | O | 0..1 | 307 Temporary Redirect | Temporary redirection.
(NOTE 2)
RedirectResponse | O | 0..1 | 308 Permanent Redirect | Permanent redirection.
(NOTE 2)
ProblemDetails | O | 0..1 | 404 Not Found | This represents the case when the subscription resource is unavailable.
NOTE 1:	The mandatory HTTP error status codes for the DELETE method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the DELETE method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the DELETE method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the DELETE method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the DELETE method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4].
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Data type | P | Cardinality | Description
PatchDocument | M | 1 | This IE contains the information regarding the JSON patch instructions for updating the NssfEventSubscriptionCreateData.
Data type | P | Cardinality | Response
codes | Description
NssfEventSubscriptionCreatedData | M | 1 | 200 OK | This case represents a successful update of the subscription.
RedirectResponse | O | 0..1 | 307 Temporary Redirect | Temporary redirection.
(NOTE 2)
RedirectResponse | O | 0..1 | 308 Permanent Redirect | Permanent redirection.
(NOTE 2)
ProblemDetails | O | 0..1 | 404 Not Found | Indicates the modification of subscription has failed due to application error.

The "cause" attribute may be used to indicate one of the following application errors:
-	SUBSCRIPTION_NOT_FOUND.
NOTE 1:	The mandatory HTTP error status codes for the PATCH method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the PATCH method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the PATCH method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the PATCH method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the PATCH method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4].
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Name | Data type | Definition
apiRoot | string | See clause 6.2.1
Name | Data type | P | Cardinality | Description
n/a |  |  |  | 
Data type | P | Cardinality | Description
n/a |  |  | 
Data type | P | Cardinality | Response
codes | Description
n/a |  |  | 200 OK | 
RedirectResponse | O | 0..1 | 307 Temporary Redirect | Temporary redirection.
(NOTE 2)
RedirectResponse | O | 0..1 | 308 Permanent Redirect | Permanent redirection.
(NOTE 2)
ProblemDetails | O | 0..1 | 405 Method Not Allowed | 
ProblemDetails | O | 0..1 | 501 Not Implemented | 
NOTE 1:	The mandatory HTTP error status codes for the OPTIONS method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the OPTIONS method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the OPTIONS method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the OPTIONS method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the OPTIONS method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4].
Name | Data type | P | Cardinality | Description
Accept-Encoding | string | O | 0..1 | Accept-Encoding, described in IETF RFC 9110 [18]
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | An alternative URI of the resource located on an alternative service instance within the same NSSF or NSSF (service) set.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Notification | Callback URI | HTTP method or custom operation | Description
(service operation)
NSSAI Availability Notification | {nfNssaiAvailabilityUri} | POST | 
Resource name | Callback URI | HTTP method or custom operation | Description
NSSAI Availability Notification Callback | {nfNssaiAvailabilityUri} | POST | The NSSF uses this callback URI to Update the AMF with any S-NSSAIs restricted per TA in the serving PLMN of the UE or to notify the Network Slice Replacement or Network Slice Instance Replacement event.
Data type | P | Cardinality | Description
NssfEventNotification | M | 1 | Representation of the data to be sent to the NF service consumer (e.g. AMF or V-NSSF).
Data type | P | Cardinality | Response
codes | Description
n/a |  |  | 204 No Content | This case represents a successful handling of notification in the NF service consumer (e.g. AMF or V-NSSF).
RedirectResponse | O | 0..1 | 307 Temporary Redirect | Temporary redirection. In the former case, the URI shall be an URI pointing to the endpoint of another NF service consumer to which the notification should be sent.
(NOTE 2)
RedirectResponse | O | 0..1 | 308 Permanent Redirect | Permanent redirection. In the former case, the URI shall be an URI pointing to the endpoint of another NF service consumer to which the notification should be sent.
(NOTE 2)
ProblemDetails | O | 0..1 | 400 Bad Request | The "cause" attribute may be used to indicate one of the following application errors:
- RESOURCE_CONTEXT_NOT_FOUND

See table 6.2.7.3-1 for the description of this error.
ProblemDetails | O | 0..1 | 404 Not Found | The "cause" attribute may be used to indicate one of the following application errors:
- RESOURCE_URI_STRUCTURE_NOT_FOUND

See table 6.2.7.3-1 for the description of this error.
NOTE 1:	The mandatory HTTP error status codes for the POST method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the POST method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the POST method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the POST method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4]. | NOTE 1:	The mandatory HTTP error status codes for the POST method listed in Table 5.2.7.1-1 of 3GPP TS 29.500 [4] other than those specified in the table above also apply, with a ProblemDetails data type (see clause 5.2.7 of 3GPP TS 29.500 [4]).
NOTE 2:	RedirectResponse may be inserted by an SCP, see clause 6.10.9.1 of 3GPP TS 29.500 [4].
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | A URI pointing to the endpoint of another NF service consumer to which the notification should be sent.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Name | Data type | P | Cardinality | Description
Location | string | M | 1 | A URI pointing to the endpoint of another NF service consumer to which the notification should be sent.
For the case, when a request is redirected to the same target resource via a different SCP, see clause 6.10.9.1 in 3GPP TS 29.500 [4].
3gpp-Sbi-Target-Nf-Id | string | O | 0..1 | Identifier of the target NF (service) instance ID towards which the request is redirected
Data type | Clause defined | Description
NssaiAvailabilityInfo | 6.2.6.2.2 | This contains the Nssai availability information requested by the AMF.
SupportedNssaiAvailabilityData | 6.2.6.2.3 | This contains the Nssai availability data information per TA supported by the AMF.
AuthorizedNssaiAvailabilityData | 6.2.6.2.4 | This contains the Nssai availability data information per TA authorized by the NSSF
RestrictedSnssai | 6.2.6.2.5 | This contains the restricted SNssai information per PLMN.
AuthorizedNssaiAvailabilityInfo | 6.2.6.2.6 | This contains the Nssai availability data information authorized by the NSSF
PatchDocument | 6.2.6.2.7 | This contains the JSON Patch instructions for updating the subscription at the NSSF.
NssfEventSubscriptionCreateData | 6.2.6.2.8 | This contains the information for event subscription.
NssfEventSubscriptionCreatedData | 6.2.6.2.9 | This contains the information for created event subscription.
NssfEventNotification | 6.2.6.2.10 | This contains the notification for created event subscription.
SnssaiReplacementSubscribeInfo | 6.2.6.2.11 | This contains the input requirements for Network Slice Replacement event subscription.
NsiUnavailabilitySubscribeInfo | 6.2.6.2.12 | This contains the input requirements for Network Slice Instance Replacement event subscription.
NssfEventType | 6.2.6.3.3 | This contains the event for the subscription.
Data type | Reference | Comments
SupportedFeatures | 3GPP TS 29.571 [7] | Used to negotiate the applicability of the features defined in table 6.2.8-1.
Snssai | 3GPP TS 29.571 [7] | 
PatchItem | 3GPP TS 29.571 [7] | Identifies the JSON Patch instructions
DateTime | 3GPP TS 29.571 [7] | 
RedirectResponse | 3GPP TS 29.571 [7] | 
NfInstanceId | 3GPP TS 29.571 [7] | 
ExtSnssai | 3GPP TS 29.571 [7] | 
NsagId | 3GPP TS 29.571 [7] | Network Slice AS Group ID
SnssaiReplaceInfo | 3GPP TS 29.571 [7] | Alternative S-NSSAI information
PlmnId | 3GPP TS 29.571 [7] | 
TaiRange | 3GPP TS 29.510 [13] | 
NFType | 3GPP TS 29.510 [13] | 
NsagInfo | 3GPP TS 29.531 | See clause 6.1.6.2.14
NsiId | 3GPP TS 29.531 | See clause 6.1.6.3.2
RecurTime | 3GPP TS 29.503 [23] | Recurrent Time
Attribute name | Data type | P | Cardinality | Description
supportedNssaiAvailabilityData | array(SupportedNssaiAvailabilityData) | M | 1..N | This IE shall contain the information regarding the S-NSSAIs the NF service consumer (e.g. AMF) and the 5G-AN supports per TA.
supportedFeatures | SupportedFeatures | C | 0..1 | This IE shall be present if at least one feature defined in clause 6.2.8 is supported
amfSetId | string | O | 0..1 | This IE may be included to indicate the AMF set identifier for the AMFs serving the TAIs where the NSSAI is available.
When present, this IE shall be constructed from PLMN-ID (i.e. three decimal digits MCC and two or three decimal digits MNC), AMF Region Id (8 bit), and AMF Set Id (10 bit).

Pattern: '^[0-9]{3}-[0-9]{2,3}-[A-Fa-f0-9]{2}-[0-3][A-Fa-f0-9]{2}$'
Attribute name | Data type | P | Cardinality | Description | Applicability
tai | Tai | M | 1 | This IE shall contain the identifier of the Tracking Area | 
supportedSnssaiList | array(ExtSnssai) | M | 1..N | This IE shall contain the S-NSSAI(s) supported by the AMF for the TA. | 
taiList | array(Tai) | O | 1..N | When present, this IE shall contain additional TAIs with the same list of supported S-NSSAIs.
(NOTE) | ONSSAI
taiRangeList | array(TaiRange) | O | 1..N | When present, this IE shall contain range(s) of TAIs with the same list of supported S-NSSAIs.
(NOTE) | ONSSAI
nsagInfos | array(NsagInfo) | O | 1..N | When present, this IE shall contain the associations between NSAGs and S-NSSAIs. | 
NOTE:	The taiList IE shall not include the TAI contained in the tai IE. The taiRangeList IE may encompass the TAI contained in the tai IE. | NOTE:	The taiList IE shall not include the TAI contained in the tai IE. The taiRangeList IE may encompass the TAI contained in the tai IE. | NOTE:	The taiList IE shall not include the TAI contained in the tai IE. The taiRangeList IE may encompass the TAI contained in the tai IE. | NOTE:	The taiList IE shall not include the TAI contained in the tai IE. The taiRangeList IE may encompass the TAI contained in the tai IE. | NOTE:	The taiList IE shall not include the TAI contained in the tai IE. The taiRangeList IE may encompass the TAI contained in the tai IE. | NOTE:	The taiList IE shall not include the TAI contained in the tai IE. The taiRangeList IE may encompass the TAI contained in the tai IE.
Attribute name | Data type | P | Cardinality | Description | Applicability
tai | Tai | M | 1 | This IE shall contain the identifier of the Tracking Area. | 
supportedSnssaiList | array(ExtSnssai) | M | 1..N | This IE shall contain the S-NSSAI(s) supported by the AMF and 5G-AN and authorized by the NSSF for the TA. | 
restrictedSnssaiList | array(RestrictedSnssai) | O | 1..N | This IE may contain the restricted S-NSSAI(s) per PLMN for the TA. If the restricted S-NSSAI is not present, the S-NSSAIs indicated in supportedSnssaiList are not restricted in this TA for any PLMN. When present, this IE shall be included only by the NSSF. | 
taiList | array(Tai) | O | 1..N | When present, this IE shall contain additional TAIs with the same lists of supported and restricted S-NSSAIs.
(NOTE) | ONSSAI
taiRangeList | array(TaiRange) | O | 1..N | When present, this IE shall contain range(s) of TAIs with the same lists of supported and restricted S-NSSAIs.
(NOTE) | ONSSAI
nsagInfos | array(NsagInfo) | O | 1..N | When present, this IE shall contain the associations between NSAGs and S-NSSAIs. | 
NOTE:	The taiList IE shall not include the TAI contained in the tai IE. The taiRangeList IE may encompass the TAI contained in the tai IE. | NOTE:	The taiList IE shall not include the TAI contained in the tai IE. The taiRangeList IE may encompass the TAI contained in the tai IE. | NOTE:	The taiList IE shall not include the TAI contained in the tai IE. The taiRangeList IE may encompass the TAI contained in the tai IE. | NOTE:	The taiList IE shall not include the TAI contained in the tai IE. The taiRangeList IE may encompass the TAI contained in the tai IE. | NOTE:	The taiList IE shall not include the TAI contained in the tai IE. The taiRangeList IE may encompass the TAI contained in the tai IE. | NOTE:	The taiList IE shall not include the TAI contained in the tai IE. The taiRangeList IE may encompass the TAI contained in the tai IE.
Attribute name | Data type | P | Cardinality | Description | Applicability
homePlmnId | PlmnId | M | 1 | This IE shall contain the home PLMN ID of the PLMN with which the serving network has roaming agreement.
This IE shall be ignored if the roamingRestriction is set to "true". | 
sNssaiList | array(ExtSnssai) | M | 1..N | This IE shall contain the array of restricted S-NSSAIs for the home PLMN Id. | 
homePlmnIdList | array(PlmnId) | O | 1..N | When present, this IE shall contain additional home PLMN IDs with which the serving network has roaming agreement and with the same list of restricted S-NSSAIs. | ONSSAI
roamingRestriction | boolean | O | 0..1 | When present, it shall be set as follows:
-	true: the list of restricted S-NSSAIs are applicable to all of the home PLMN IDs with which the serving network has roaming agreement;
-	false (default): the list of restricted S-NSSAIs are applicable to part of the home PLMN IDs with which the serving network has roaming agreement as included in the homePlmnId and homePlmnIdList IEs. | ONSSAI
Attribute name | Data type | P | Cardinality | Description
authorizedNssaiAvailabilityData | array(AuthorizedNssaiAvailabilityData) | M | 1..N | Contains the authorized NSSAI availability information.
supportedFeatures | SupportedFeatures | C | 0..1 | This IE shall be present if at least one feature defined in clause 6.2.8 is supported
Attribute name | Data type | P | Cardinality | Description
N/A | array(PatchItem) | M | 1..N | An array of patch instructions to update the NSSAI availability information or the NssfEventSubscriptionCreateData at the NSSF. See 3GPP TS 29.571 [7].
Attribute name | Data type | P | Cardinality | Description | Applicability
nfNssaiAvailabilityUri | Uri | M | 1 | Identifies the recipient of notifications sent by the NF service consumer (e.g. AMF, V-NSSF) for this subscription | 
taiList | array(Tai) | C | 0..N | When present, this IE shall identify the TAIs supported by the NF service consumer (e.g. AMF).
This IE shall be present if the NF Service Consumer subscribes to the "SNSSAI_STATUS_CHANGE_REPORT" event for this subscription.
(NOTE) | 
event | NssfEventType | M | 1 | Describes the event to be subscribed for this subscription. | 
additionalEvents | array(NssfEventType) | C | 1..N | This IE shall be present if the NF Service Consumer wishes to subscribe to more than one event types. When present, this IE shall indicate the additional event(s) requested to be subscribed. | 
expiry | DateTime | O | 0..1 | This IE may be included by the NF service consumer. When present, this IE shall represent the suggested UTC time after which the subscription becomes invalid. | 
amfSetId | string | O | 0..1 | This IE may be included for "SNSSAI_STATUS_CHANGE_REPORT" event, to identify a specific AMF Set for which this subscription applies.

When present, this IE shall be constructed from PLMN-ID (i.e. three decimal digits MCC and two or three decimal digits MNC), AMF Region Id (8 bit), and AMF Set Id (10 bit).

Pattern: '^[0-9]{3}-[0-9]{2,3}-[A-Fa-f0-9]{2}-[0-3][A-Fa-f0-9]{2}$' | 
taiRangeList | array(TaiRange) | O | 1..N | Identifies a list of TAI ranges supported by the NF service consumer (e.g. AMF) to be applied for "SNSSAI_STATUS_CHANGE_REPORT" event.
The NF service consumer shall only include this IE when it knows that the NSSF supports the "ONSSAI" feature.
(NOTE) | ONSSAI
amfId | NfInstanceId | O | 0..1 | This IE may be included to indicate the instance identity of the network function creating the subscription for "SNSSAI_STATUS_CHANGE_REPORT" event. | 
supportedFeatures | SupportedFeatures | C | 0..1 | This IE shall be present if at least one feature defined in clause 6.2.8 is supported. | 
allAmfSetTaiInd | boolean | O | 0..1 | This IE may be present when amfSetId is present.

When present, this IE shall indicate whether the subscription is for all TAIs of the AMF set:
- ture: all TAIs of the AMF Set is subscribed.
- false (default): indicated TAIs (in the taiList IE and/or taiRangeList IE) are subscribed. | SATAS
nsrpSubscribeInfo | SnssaiReplacementSubscribeInfo | C | 0..1 | This IE shall be present when the NF Service Consumer subscribes to the Network Slice Replacement event. | 
nsiunSubscribeInfo | NsiUnavailabilitySubscribeInfo | C | 0..1 | This IE shall be present when the NF Service Consumer subscribes to the Network Slice Instance Replacement event. | 
validityTimeSubList | array(Snssai) | C | 1..N | This IE shall be present when the NF Service Consumer subscribes to "SNSSAI_VALIDITY_TIME_REPORT" event.

When present, this IE shall include the list of S-NSSAIs to be subscribed. | 
NOTE:	For event "SNSSAI_STATUS_CHANGE_REPORT", the taiList attribute shall only be set to an empty array if the NSSF supports the "ONSSAI" feature and taiRangeList IE is present, or if the NSSF supports the "SATAS" feature and the allAmfSetTaiInd IE is present with value true. A NF service consumer (e.g. AMF) may provide both taiRangeList and taiList attributes, to carry individual TAI(s) in the taiList attribute and ranges of TAIs in the taiRangeList attribute. | NOTE:	For event "SNSSAI_STATUS_CHANGE_REPORT", the taiList attribute shall only be set to an empty array if the NSSF supports the "ONSSAI" feature and taiRangeList IE is present, or if the NSSF supports the "SATAS" feature and the allAmfSetTaiInd IE is present with value true. A NF service consumer (e.g. AMF) may provide both taiRangeList and taiList attributes, to carry individual TAI(s) in the taiList attribute and ranges of TAIs in the taiRangeList attribute. | NOTE:	For event "SNSSAI_STATUS_CHANGE_REPORT", the taiList attribute shall only be set to an empty array if the NSSF supports the "ONSSAI" feature and taiRangeList IE is present, or if the NSSF supports the "SATAS" feature and the allAmfSetTaiInd IE is present with value true. A NF service consumer (e.g. AMF) may provide both taiRangeList and taiList attributes, to carry individual TAI(s) in the taiList attribute and ranges of TAIs in the taiRangeList attribute. | NOTE:	For event "SNSSAI_STATUS_CHANGE_REPORT", the taiList attribute shall only be set to an empty array if the NSSF supports the "ONSSAI" feature and taiRangeList IE is present, or if the NSSF supports the "SATAS" feature and the allAmfSetTaiInd IE is present with value true. A NF service consumer (e.g. AMF) may provide both taiRangeList and taiList attributes, to carry individual TAI(s) in the taiList attribute and ranges of TAIs in the taiRangeList attribute. | NOTE:	For event "SNSSAI_STATUS_CHANGE_REPORT", the taiList attribute shall only be set to an empty array if the NSSF supports the "ONSSAI" feature and taiRangeList IE is present, or if the NSSF supports the "SATAS" feature and the allAmfSetTaiInd IE is present with value true. A NF service consumer (e.g. AMF) may provide both taiRangeList and taiList attributes, to carry individual TAI(s) in the taiList attribute and ranges of TAIs in the taiRangeList attribute. | NOTE:	For event "SNSSAI_STATUS_CHANGE_REPORT", the taiList attribute shall only be set to an empty array if the NSSF supports the "ONSSAI" feature and taiRangeList IE is present, or if the NSSF supports the "SATAS" feature and the allAmfSetTaiInd IE is present with value true. A NF service consumer (e.g. AMF) may provide both taiRangeList and taiList attributes, to carry individual TAI(s) in the taiList attribute and ranges of TAIs in the taiRangeList attribute.
Attribute name | Data type | P | Cardinality | Description
subscriptionId | string | M | 1 | Identifies the subscription Id for the created subscription.
expiry | DateTime | C | 0..1 | This IE shall be included, if, based on operator policy and taking into account the expiry time included in the request, the NSSF needs to include an expiry time. When present, it represents the UTC time after which the subscribed event shall stop generating report and the subscription becomes invalid. Upon reaching this expiry time the NF service consumer shall delete the representation of the subscription it may have.
authorizedNssaiAvailabilityData | array(AuthorizedNssaiAvailabilityData) | O | 1..N | This IE may be included if the NF Service Consumer requested to subscribe to changes in the status of NSSAI availability information and if the authorized NSSAI availability (i.e. S-NSSAIs available per TA (unrestricted) and any S-NSSAIs restricted per PLMN in that TA in the serving PLMN of the UE) is available.
supportedFeatures | SupportedFeatures | C | 0..1 | This IE shall be present if at least one feature defined in clause 6.2.8 is supported.
acceptedEvents | array(NssfEventType) | O | 1..N | This IE may be present when the request is subscribing to more than one events.

When present, this IE shall indicate the events that are accepted by the NSSF for this subscription.
Attribute name | Data type | P | Cardinality | Description | Applicability
subscriptionId | string | M | 1 | Indicates which subscription generated event notificaiton. This parameter shall be generated by NSSF and returned in "Location" header in HTTP responses. This IE may be used by the NF service consumer to correlate the received notification with the subscription if a NF service consumer uses a common call-back URI for multiple subscriptions. | 
authorizedNssaiAvailabilityData | array(AuthorizedNssaiAvailabilityData) | C | 0..N | This IE shall be present for a notification of changes in the status of the NSSAI availability information. When present, this IE shall contain the authorized NSSAI availability information for all TAs the AMF subscribed to. Each element shall contain the current status of the list of S-NSSAI available in a TA and the list of S-NSSAI restricted per PLMN in that TA.

The NF Service Consumer shall replace any authorizedNssaiAvailabilityData received earlier by the new authorizedNssaiAvailabilityData received in the notification.

When no supported S-NSSAIs authorized by the NSSF for all TAs, this IE shall contain an empty array indicating Authorized NSSAI Availability information is empty. When received this IE with empty array, the NF Service Consumer shall remove any locally stored authorizedNssaiAvailabilityData previously received from NSSF. (NOTE 1) | 
altNssai | array(SnssaiReplaceInfo) | C | 1..N | The IE shall be present for a notification of Network Slice Replacement. When present, this IE shall indicate the impacted S-NSSAIs if one or more of S-NSSAIs availability status changed from available to not available and vice versa, and the current status for each reported S-NSSAI.

This IE may contain the alternative S-NSSAI per impacted S-NSSAI for the S-NSSAIs that are reported as being not available.
See clause 5.15.19 in 3GPP TS 23.501 [2]. | NSRP
unavailableNsiList | array(NsiId) | C | 1..N | This IE shall be present for a of Network Slice Instance Replacement. When present, this IE shall indicate the NSI IDs for which the status has changed (e.g., that are congested or no longer available). | NSIUN
nssaiValidityTimeInfo | map(DateTime) | C | 1..N | The IE shall be present for a notification of NSSAI validity time information.
A map (list of key-value pairs where Snssai converted to string serves as key; see 3GPP TS 29.571 [7] clause 5.4.4.2) of the current validity time.
(NOTE 2) | 
nssaiValidityTimeInfoList | map(array(RecurTime)) | C | 1..N(1..M) | The IE shall be present for a notification of NSSAI validity time information.
A map (list of key-value pairs where Snssai converted to string serves as key; see 3GPP TS 29.571 [7] clause 5.4.4.2) of the S-NSSAI validity time information, with each map item indicating the validity time as a list of time period(s) for the corresponding S-NSSAI. | 
NOTE 1:	For event "SNSSAI_STATUS_CHANGE_REPORT", the NSSF shall only send notification with empty array to NF Service Consumer previously indicated support of "EANAN" feature, when there is no supported S-NSSAIs authorized by the NSSF for all TAs.
NOTE 2:	This IE is deprecated. To notify the validity time information for temporarily available S-NSSAI(s) the nssaiValidityTimeInfoList IE shall be used. | NOTE 1:	For event "SNSSAI_STATUS_CHANGE_REPORT", the NSSF shall only send notification with empty array to NF Service Consumer previously indicated support of "EANAN" feature, when there is no supported S-NSSAIs authorized by the NSSF for all TAs.
NOTE 2:	This IE is deprecated. To notify the validity time information for temporarily available S-NSSAI(s) the nssaiValidityTimeInfoList IE shall be used. | NOTE 1:	For event "SNSSAI_STATUS_CHANGE_REPORT", the NSSF shall only send notification with empty array to NF Service Consumer previously indicated support of "EANAN" feature, when there is no supported S-NSSAIs authorized by the NSSF for all TAs.
NOTE 2:	This IE is deprecated. To notify the validity time information for temporarily available S-NSSAI(s) the nssaiValidityTimeInfoList IE shall be used. | NOTE 1:	For event "SNSSAI_STATUS_CHANGE_REPORT", the NSSF shall only send notification with empty array to NF Service Consumer previously indicated support of "EANAN" feature, when there is no supported S-NSSAIs authorized by the NSSF for all TAs.
NOTE 2:	This IE is deprecated. To notify the validity time information for temporarily available S-NSSAI(s) the nssaiValidityTimeInfoList IE shall be used. | NOTE 1:	For event "SNSSAI_STATUS_CHANGE_REPORT", the NSSF shall only send notification with empty array to NF Service Consumer previously indicated support of "EANAN" feature, when there is no supported S-NSSAIs authorized by the NSSF for all TAs.
NOTE 2:	This IE is deprecated. To notify the validity time information for temporarily available S-NSSAI(s) the nssaiValidityTimeInfoList IE shall be used. | 
Attribute name | Data type | P | Cardinality | Description
snssaiToSubscribe | array(Snssai) | M | 0..N | This IE shall indicate the S-NSSAIs for which notification is requested during Network Slice Replacement.

In the case of roaming it shall indicate:
- the VPLMN S-NSSAIs for which notification is requested in case of Network Slice Replacement; or
- the HPLMN S-NSSAIs for which notification is requested in case of Network Slice Replacement. (NOTE)
nfType | NFType | M | 1 | This IE shall contain the NF type of the NF service consumer.
nfId | NfInstanceId | M | 1 | This IE shall contain the NF identifier of the NF service consumer.
plmnId | PlmnId | C | 0..1 | This IE shall be present in roaming scenarios, if the indicated S-NSSAI is an HPLMN S-NSSAI. It may be present otherwise. When present, it shall indicate the PLMN ID of the S-NSSAI.
NOTE:	The snssaiToSubscribe attribute shall be set to an empty array for a Network Slice Replacement subscription applying to all S-NSSAIs. | NOTE:	The snssaiToSubscribe attribute shall be set to an empty array for a Network Slice Replacement subscription applying to all S-NSSAIs. | NOTE:	The snssaiToSubscribe attribute shall be set to an empty array for a Network Slice Replacement subscription applying to all S-NSSAIs. | NOTE:	The snssaiToSubscribe attribute shall be set to an empty array for a Network Slice Replacement subscription applying to all S-NSSAIs. | NOTE:	The snssaiToSubscribe attribute shall be set to an empty array for a Network Slice Replacement subscription applying to all S-NSSAIs.
Attribute name | Data type | P | Cardinality | Description
nsiToSubscribe | array(NsiId) | O | 0..N | When present, this IE shall indicate the identifier of the Network Slice Instance(s) for which notifications are requested in case the status of the NSI changes (e.g., becomes congested or no longer available). (NOTE 1) (NOTE 3)
snssaiToSubscribe | array(Snssai) | O | 0..N | When present, this IE shall indicate the identifier of the S-NSSAI related to the NSI for which notifications shall be invoked in case the NSI becomes congested or no longer available.(NOTE 2) (NOTE 3)
NOTE 1:	The nsiToSubscribe attribute shall be set to an empty array for a Network Slice Instance Replacement subscription applying to all NSIs.
NOTE 2:	The snssaiToSubscribe attribute shall be set to an empty array for a Network Slice Instance Replacement subscription applying to all S-NSSAIs.
NOTE 3:	At least one of the nsiToSubscribe IE or snssaiToSubscribe IE shall be present. | NOTE 1:	The nsiToSubscribe attribute shall be set to an empty array for a Network Slice Instance Replacement subscription applying to all NSIs.
NOTE 2:	The snssaiToSubscribe attribute shall be set to an empty array for a Network Slice Instance Replacement subscription applying to all S-NSSAIs.
NOTE 3:	At least one of the nsiToSubscribe IE or snssaiToSubscribe IE shall be present. | NOTE 1:	The nsiToSubscribe attribute shall be set to an empty array for a Network Slice Instance Replacement subscription applying to all NSIs.
NOTE 2:	The snssaiToSubscribe attribute shall be set to an empty array for a Network Slice Instance Replacement subscription applying to all S-NSSAIs.
NOTE 3:	At least one of the nsiToSubscribe IE or snssaiToSubscribe IE shall be present. | NOTE 1:	The nsiToSubscribe attribute shall be set to an empty array for a Network Slice Instance Replacement subscription applying to all NSIs.
NOTE 2:	The snssaiToSubscribe attribute shall be set to an empty array for a Network Slice Instance Replacement subscription applying to all S-NSSAIs.
NOTE 3:	At least one of the nsiToSubscribe IE or snssaiToSubscribe IE shall be present. | NOTE 1:	The nsiToSubscribe attribute shall be set to an empty array for a Network Slice Instance Replacement subscription applying to all NSIs.
NOTE 2:	The snssaiToSubscribe attribute shall be set to an empty array for a Network Slice Instance Replacement subscription applying to all S-NSSAIs.
NOTE 3:	At least one of the nsiToSubscribe IE or snssaiToSubscribe IE shall be present.
Type Name | Type Definition | Description
 | <one simple data type, e.g. boolean, integer, null, number, string> | 
Enumeration value | Description
"SNSSAI_STATUS_CHANGE_REPORT" | A NF subscribes to this event to receive the status change about the current S-NSSAI(s) available (i.e unrestricted) per TA and the status change about the list of restricted S-NSSAI(s) per TA and per PLMN in the serving PLMN of the UE.
"SNSSAI_REPLACEMENT_REPORT" | A NF subscribes to this event to receive a replacement S-NSSAI for each impacted S-NSSAI. See clause 5.15.19 of 3GPP TS 23.501 [2].
"NSI_UNAVAILAIBILITY_REPORT" | A NF subscribes to this event to receive the list of unavailable NSIs (e.g., due to overload). See clause 5.15.20 of 3GPP TS 23.501 [2].
"SNSSAI_VALIDITY_TIME_REPORT" | A NF subscribes to this event to receive the status change about NSSAI validity time information. See clause 5.2.16.3.4 of 3GPP TS 23.502 [3].
Application Error | HTTP status code | Description
RESOURCE_CONTEXT_NOT_FOUND | 400 Bad Request | Indicates that the NF Service Consumer (e.g. AMF) received a notification request from NSSF on an existing callback URI, but the corresponding context does not exist at the NF Service Consumer.
SNSSAI_NOT_SUPPORTED | 403 Forbidden | The request is rejected due to the SNSSAI provided in the request is not supported in the PLMN.
NOT_AUTHORIZED | 403 Forbidden | The request is rejected due to the NF service consumer is not authorized to update the NSSAI availability information, or subscribe for the NSSAI availability information notification.
RESOURCE_NOT_FOUND | 404 Not Found | The request is rejected due to the NF service consumer is authorized, but the resource related to the NF Id for which the NSSAI availability information is updated or deleted is unavailable.
SUBSCRIPTION_NOT_FOUND | 404 Not Found | Indicates the modification of subscription has failed due to an application error when the subscription is not found in the NSSF.
RESOURCE_URI_STRUCTURE_NOT_FOUND | 404 Not Found | Indicates that the NF Service Consumer (e.g. AMF) received a notification request from NSSF on a callback URI that is not known to the NF Service Consumer.
UNSUPPORTED_EVENT_TYPE | 501 Not Implemented | The request for creation of a subscription is rejected because none of the events is supported by the NSSF.
Feature Number | Feature | M/O | Description
1 | ONSSAI | O | Optimized NSSAI Availability Data encoding

When this feature is supported:
-	NSSAI Availability data may be signalled per list or range(s) of TAIs (see clauses 6.2.6.2.3 and 6.2.6.2.4); and
-	RestrictedSnssai may encode a list of Home PLMN IDs or may be applicable to all of the Home PLMN IDs (see clause 6.2.6.2.5).
-	NSSF event subscription may encode a list of TAI ranges (see clause 6.2.6.2.8).
2 | SUMOD | O | Subscription Modification in Subscribe Service Operation

When this feature is supported, the subscription of NSSAI availability information is supported to be modified (see clause 5.3.2.3.2).
3 | EANAN | O | Empty Authorized NSSAI Availability Notification

A NSSF supporting this feature shall send a notification to NF consumer (as subscriber) with empty array of Authorized NSSAI Availability Data, when no supported NSSAI Authorized by the NSSF for all TAs after latest update and the NF consumer indicated support of this feature.

A NF Consumer support this feature shall accept empty array of Authorized NSSAI Availability Data in a notification from NSSF and delete locally stored Authorized NSSAI Availability Data previously received.
4 | ES3XX | M | Extended Support of HTTP 307/308 redirection

An NF Service Consumer (e.g. AMF) that supports this feature shall support handling of HTTP 307/308 redirection for any service operation of the Nnssf_NSSAIAvailability service. An NF Service Consumer that does not support this feature does only support HTTP redirection as specified for 3GPP Release 15.
5 | SATAS | O | Subscribe ALL TAIs for AMF Set

A NSSF supporting this feature shall support the NF Consumer to subscribe to all TAI(s) for an AMF set.
6 | NSIUN | O | Network Slice Instance Unavailability Notification

An NF Service Consumer (e.g., AMF, V-NSSF) and NSSF supporting this feature shall support notifications from the NSSF to the NF Service Consumer about the unavailability of Network Slice Instances, as specified in clause 5.15.5.3 of 3GPP TS 23.501 [2].
7 | NSRP | O | Network Slice Replacement

An NF service consumer (e.g., AMF, V-NSSF) that supports this feature shall support network slice replacement as specified in clause 5.15.19 of 3GPP TS 23.501 [2].
Feature number: The order number of the feature within the supportedFeatures attribute (starting with 1).
Feature: A short name that can be used to refer to the bit and to the feature.
M/O: Defines if the implementation of the feature is mandatory ("M") or optional ("O").
Description: A clear textual description of the feature. | Feature number: The order number of the feature within the supportedFeatures attribute (starting with 1).
Feature: A short name that can be used to refer to the bit and to the feature.
M/O: Defines if the implementation of the feature is mandatory ("M") or optional ("O").
Description: A clear textual description of the feature. | Feature number: The order number of the feature within the supportedFeatures attribute (starting with 1).
Feature: A short name that can be used to refer to the bit and to the feature.
M/O: Defines if the implementation of the feature is mandatory ("M") or optional ("O").
Description: A clear textual description of the feature. | Feature number: The order number of the feature within the supportedFeatures attribute (starting with 1).
Feature: A short name that can be used to refer to the bit and to the feature.
M/O: Defines if the implementation of the feature is mandatory ("M") or optional ("O").
Description: A clear textual description of the feature.
Change history | Change history | Change history | Change history | Change history | Change history | Change history | Change history
Date | Meeting | TDoc | CR | Rev | Cat | Subject/Comment | New version
2017-10 | CT4#80 | C4-175279 |  |  |  | Initial Draft. | 0.1.0
2017-10 | CT4#81 | C4-175398 |  |  |  | Implementation of C4-175280 | 0.2.0
2018-01 | CT4#82 | C4-181394 |  |  |  | Implementation of C4-181240、C4-181242、C4-181244、C4-181355、C4-181356、C4-181357 | 0.3.0
2018-03 | CT4#83 | C4-182438 |  |  |  | Implementation of C4-182087、C4-182294、C4-182295、C4-182296、C4-182297、C4-182298、C4-182299 | 0.4.0
2018-03 | CT#79 | CP-180035 |  |  |  | Presented for information | 1.0.0
2018-04 | CT4#84 | C4-183519 |  |  |  | Implementation of C4-183068、C4-183071、C4-183431、C4-183432、C4-183433 | 1.1.0
2018-05 | CT4#85 | C4-184631 |  |  |  | Implementation of C4-184602, C4-184023, C4-184024, C4-184025, C4-184026, C4-184603, C4-184527, C4-184528, C4-184604, C4-184632 | 1.2.0
2018-06 | CT#80 | CP-181108 |  |  |  | Presented for approval | 2.0.0
2018-06 | CT#80 |  |  |  |  | Approved in CT#80. | 15.0.0
2018-09 | CT#81 | CP-182160 | 0001 | 5 | F | Alignment of Nnssf_NSSelection_Get service operation with stage 2 | 15.1.0
2018-09 | CT#81 | CP-182014 | 0002 | 2 | F | Adding NRF corresponding to an AMF set | 15.1.0
2018-09 | CT#81 | CP-182167 | 0003 | 4 | F | Corrections to NSSF Data Types | 15.1.0
2018-09 | CT#81 | CP-182063 | 0004 |  | F | Corrections to NSSAIAvailability Service Operations | 15.1.0
2018-09 | CT#81 | CP-182063 | 0005 | 1 | F | Configured NSSAI for HPLMN - Alignment with Stage 2 | 15.1.0
2018-09 | CT#81 | CP-182063 | 0006 |  | F | Correction to NRF Id in NSIInformation | 15.1.0
2018-09 | CT#81 | CP-182063 | 0007 |  | F | Description of Structured data types | 15.1.0
2018-09 | CT#81 | CP-182063 | 0008 |  | F | API version number update | 15.1.0
2018-12 | CT#82 | CP-183022 | 0009 |  | F | Type Definition of AllowedNssai | 15.2.0
2018-12 | CT#82 | CP-183022 | 0010 | 1 | F | Correction to Slice Information For Registration | 15.2.0
2018-12 | CT#82 | CP-183022 | 0011 |  | F | API Root | 15.2.0
2018-12 | CT#82 | CP-183022 | 0012 | 3 | F | Common Error Status Codes | 15.2.0
2018-12 | CT#82 | CP-183148 | 0013 | 2 | F | Array Range Correction | 15.2.0
2018-12 | CT#82 | CP-183022 | 0016 | 1 | F | OpenAPI Corrections | 15.2.0
2018-12 | CT#82 | CP-183022 | 0017 | 2 | F | Subscription Lifetime for NSSAI Availability Event Subscription | 15.2.0
2018-12 | CT#82 | CP-183022 | 0018 |  | F | Correction of Resource URI structure | 15.2.0
2018-12 | CT#82 | CP-183022 | 0019 |  | F | Add Delete Service Operation in Nnssf_NSSAIAvailability Service | 15.2.0
2018-12 | CT#82 | CP-183022 | 0020 | 2 | F | Add the Default Configured NSSAI Indication in Nnssf_NSSelection Service | 15.2.0
2018-12 | CT#82 | CP-183022 | 0021 |  | F | CR 0021 29.531 Rel-15 Resource Uri Correction | 15.2.0
2018-12 | CT#82 | CP-183022 | 0022 |  | F | Correction to NssaiAvailabilityInfo | 15.2.0
2018-12 | CT#82 | CP-183022 | 0023 | 2 | F | Make OAuth2.0 Optional to Use | 15.2.0
2018-12 | CT#82 | CP-183022 | 0024 |  | F | ExternalDocs | 15.2.0
2018-12 | CT#82 | CP-183022 | 0025 |  | F | API Version | 15.2.0
2019-03 | CT#83 | CP-190027 | 0026 | 1 | F | Definition of TargetAmfSet | 15.3.0
2019-03 | CT#83 | CP-190027 | 0027 | 1 | F | OpenAPI Corrections | 15.3.0
2019-03 | CT#83 | CP-190027 | 0029 |  | F | Add missing NFType reference in reused data types | 15.3.0
2019-03 | CT#83 | CP-190027 | 0030 | 2 | F | Clarify the conditions of returning Configured NSSAI. | 15.3.0
2019-03 | CT#83 | CP-190027 | 0031 | 1 | F | Service operation of Nnssf_NSSelection service during UE configuration update procedure | 15.3.0
2019-03 | CT#83 | CP-190171 | 0032 | 1 | F | API version update | 15.3.0
2019-06 | CT#84 | CP-191039 | 0033 | 1 | F | Content encodings supported in HTTP requests | 15.4.0
2019-06 | CT#84 | CP-191039 | 0034 | 4 | F | Add AMFset in NssaiAvailabilityInfo | 15.4.0
2019-06 | CT#84 | CP-191039 | 0036 | 2 | F | Storage of OpenAPI specification files | 15.4.0
2019-06 | CT#84 | CP-191039 | 0039 | 1 | F | API URIs of the NRF | 15.4.0
2019-06 | CT#84 | CP-191039 | 0040 | 1 | F | Subscription to and notification of NSSF events | 15.4.0
2019-06 | CT#84 | CP-191039 | 0041 | 2 | F | Essential Correction on Application Error returned by NSSF | 15.4.0
2019-06 | CT#84 | CP-191039 | 0042 | 1 | F | Copyright Note in YAML file | 15.4.0
2019-06 | CT#84 | CP-191039 | 0043 |  | F | 3GPP TS 29.531 API version update | 15.4.0
2019-09 | CT#85 | CP-192111 | 0045 |  | F | Essential Correction on AllowedNssai | 15.5.0
2019-09 | CT#85 | CP-192131 | 0044 | 1 | B | Slice selection during handover from 4G to 5G | 16.0.0
2019-12 | CT#86 | CP-193048 | 0047 | 1 | B | Subscribed NSSAI from the UDM | 16.1.0
2019-12 | CT#86 | CP-193044 | 0049 |  | F | 3GPP TS 29.531 API version update | 16.1.0
2020-03 | CT#87 | CP-200039 | 0050 | 2 | F | Add Corresponding API descriptions in clause 5.1 | 16.2.0
2020-03 | CT#87 | CP-200020 | 0051 | 2 | B | Optimized NSSAI Availability Data encoding | 16.2.0
2020-03 | CT#87 | CP-200020 | 0052 | 3 | B | AMF Service Set ID in Nnssf_NSSelection response | 16.2.0
2020-03 | CT#87 | CP-200039 | 0053 | 2 | D | Editorial corrections | 16.2.0
2020-03 | CT#87 | CP-200039 | 0054 | 1 | F | Correction - formatting consistency | 16.2.0
2020-03 | CT#87 | CP-200020 | 0055 | 2 | B | 29531 CR optionality of ProblemDetails | 16.2.0
2020-03 | CT#87 | CP-200020 | 0056 | 1 | F | Modifications in the API of Nnssf_NSSAIAvailability service for the support of compression | 16.2.0
2020-03 | CT#87 | CP-200020 | 0057 | 2 | F | Corrections in the NSSF specification | 16.2.0
2020-03 | CT#87 | CP-200052 | 0058 |  | F | 3GPP TS 29.531 Rel16 API External doc update | 16.2.0
2020-07 | CT#88 | CP-201058 | 0059 |  | F | Storage of YAML files in ETSI Forge | 16.3.0
2020-07 | CT#88 | CP-201058 | 0060 | 3 | F | Supported Headers Tables for Request and Response codes | 16.3.0
2020-07 | CT#88 | CP-201058 | 0061 | 1 | F | Add a new Notifications Overview Table | 16.3.0
2020-07 | CT#88 | CP-201034 | 0062 | 1 | F | Remaining modifications in the API of Nnssf_NSSAIAvailability service for the support of compression | 16.3.0
2020-07 | CT#88 | CP-201034 | 0063 |  | F | Slice Differentiator Ranges and Wildcard | 16.3.0
2020-07 | CT#88 | CP-201058 | 0064 | 1 | B | Restricted Snssai for roaming users | 16.3.0
2020-07 | CT#88 | CP-201034 | 0065 |  | F | PATCH Response | 16.3.0
2020-07 | CT#88 | CP-201058 | 0066 | 1 | F | Data type column in Resource URI variables Table | 16.3.0
2020-07 | CT#88 | CP-201058 | 0067 |  | F | mappingOfNssai IE in SliceInfoForRegistration | 16.3.0
2020-07 | CT#88 | CP-201058 | 0068 | 1 | F | URI of the Nnssf_NSSelection and Nnssf_NSSAIAvailability Services | 16.3.0
2020-07 | CT#88 | CP-201058 | 0069 | 1 | F | Error code corrections | 16.3.0
2020-07 | CT#88 | CP-201326 | 0071 | 1 | F | 29.531 Rel-16 API version and External doc update | 16.3.0
2020-09 | CT#89 | CP-202090 | 0072 |  | F | Essential correction to event SNSSAI_STATUS_CHANGE_REPORT | 16.4.0
2020-09 | CT#89 | CP-202090 | 0073 | 1 | F | Slice selection based on Load Analytics Information from NWDAF | 16.4.0
2020-09 | CT#89 | CP-202090 | 0074 | 3 | F | TAI Range List Served by an AMF | 16.4.0
2020-09 | CT#89 | CP-202090 | 0077 | 1 | F | Request mapping of S-NSSAI | 16.4.0
2020-09 | CT#89 | CP-202090 | 0078 | 2 | F | Subscription modification | 16.4.0
2020-09 | CT#89 | CP-202035 | 0080 | 2 | F | Notify Empty Authorized NSSAI Availability | 16.4.0
2020-09 | CT#89 | CP-202096 | 0081 |  | F | 29.531 Rel-16 API version and External doc update | 16.4.0
2020-12 | CT#90-e | CP-203162 | 0082 | 1 | F | HTTP 3xx redirection | 16.5.0
2020-12 | CT#90-e | CP-203040 | 0083 | 1 | F | Mapping of S-NSSAIs in HPLMN and VPLMN | 16.5.0
2020-12 | CT#90-e | CP-203040 | 0084 | 1 | F | Number of allowed S-NSSAIs | 16.5.0
2020-12 | CT#90-e | CP-203035 | 0085 |  | F | Storage of YAML files in 3GPP Forge | 16.5.0
2020-12 | CT#90-e | CP-203036 | 0086 |  | F | API version and External doc update | 16.5.0
2021-03 | CT#91-e | CP-210043 | 0090 |  | F | OpenAPI syntax error | 16.6.0
2021-03 | CT#91-e | CP-210054 | 0091 |  | F | 29.531 Rel-16 API version and External doc update | 16.6.0
2021-03 | CT#91-e | CP-210034 | 0088 | 1 | F | OpenAPI Reference | 17.0.0
2021-03 | CT#91-e | CP-210025 | 0089 | 1 | F | N31 interface between NSSFs | 17.0.0
2021-06 | CT#92-e | CP-211083 | 0094 |  | A | Essential correction on Nssai Availability Document Update | 17.1.0
2021-06 | CT#92-e | CP-211028 | 0095 |  | F | Data Types Descriptions | 17.1.0
2021-06 | CT#92-e | CP-211059 | 0097 | 1 | F | Redirect Responses | 17.1.0
2021-06 | CT#92-e | CP-211046 | 0098 |  | F | Miscellaneous corrections | 17.1.0
2021-06 | CT#92-e | CP-211050 | 0100 |  | F | 29.531 Rel-17 API version and External doc update | 17.1.0
2021-09 | CT#93-e | CP-212075 | 0106 |  | A | Incorrect references | 17.2.0
2021-09 | CT#93-e | CP-212052 | 0101 |  | F | SNSSAI_NOT_SUPPORTED | 17.2.0
2021-09 | CT#93-e | CP-212030 | 0102 | 3 | B | NSSRG value | 17.2.0
2021-09 | CT#93-e | CP-212030 | 0103 | 3 | B | Missing indication of UE support of NSSRG functionality in NSSF service | 17.2.0
2021-09 | CT#93-e | CP-212030 | 0104 | 2 | B | Target NSSAI | 17.2.0
2021-09 | CT#93-e | CP-212045 | 0107 | 1 | F | NSSAIAvailability Notify | 17.2.0
2021-09 | CT#93-e | CP-212059 | 0108 |  | F | 29.531 Rel-17 API version and External doc update | 17.2.0
2021-12 | CT#94-e | CP-213085 | 0111 |  | B | Indicating possible use of OAuth2 authorization in NSSF response | 17.3.0
2021-12 | CT#94-e | CP-213085 | 0114 |  | F | Corrections to the API URI | 17.3.0
2021-12 | CT#94-e | CP-213086 | 0112 | 1 | F | Correction on requestedNssai | 17.3.0
2021-12 | CT#94-e | CP-213086 | 0115 |  | F | Notification Errors | 17.3.0
2021-12 | CT#94-e | CP-213092 | 0110 | 1 | B | UDM indication to provide full set of subscribed S-NSSAIs | 17.3.0
2021-12 | CT#94-e | CP-213092 | 0113 | 1 | B | Rejected S-NSSAIs for RA in NS Selection | 17.3.0
2021-12 | CT#94-e | CP-213092 | 0119 | 1 | B | Target NSSAI correction | 17.3.0
2021-12 | CT#94-e | CP-213092 | 0116 |  | F | Configured NSSAI can include S-NSSAIs with different NSSRG values | 17.3.0
2021-12 | CT#94-e | CP-213113 | 0117 | 1 | F | Configured NSSAI shall be returned by NSSF to AMF during UCU procedure | 17.3.0
2021-12 | CT#94-e | CP-213113 | 0118 | 1 | F | Clarification on the condition when AMF can retrieve slice mapping information | 17.3.0
2021-12 | CT#94-e | CP-213121 | 0120 |  | F | 29.531 Rel-17 API version and External doc update | 17.3.0
2022-03 | CT#95-e | CP-220024 | 0126 |  | F | Formatting of Description Fields | 17.4.0
2022-03 | CT#95-e | CP-220087 | 0122 |  | F | Correction on ExtSnssai | 17.4.0
2022-03 | CT#95-e | CP-220087 | 0123 |  | F | Adding use case for sending Allowed NSSAI aligned with stage 2 | 17.4.0
2022-03 | CT#95-e | CP-220092 | 0124 | 1 | D | Capitalize allowed NSSAI and target NSSAI | 17.4.0
2022-03 | CT#95-e | CP-220066 | 0127 |  | F | 29.531 Rel-17 API version and External doc update | 17.4.0
2022-06 | CT#96 | CP-221033 | 0129 | 1 | F | Redirect or handover the UE to a cell of another TA | 17.5.0
2022-06 | CT#96 | CP-221029 | 0130 | 2 | F | Clarification on targetAmfSet in AuthorizedNetworkSliceInfo | 17.5.0
2022-06 | CT#96 | CP-221055 | 0131 | 4 | F | Nnssf_NSSelection service update to support NSAG | 17.5.0
2022-06 | CT#96 | CP-221055 | 0132 | 3 | F | Nnssf_NSSAIAvailability service update to support NSAG | 17.5.0
2022-06 | CT#96 | CP-221033 | 0135 | 1 | F | Correction on OpenAPI | 17.5.0
2022-06 | CT#96 | CP-221051 | 0136 |  | F | 29.531 Rel-17 API version and External doc update | 17.5.0
2022-09 | CT#97e | CP-222055 | 0140 | 1 | F | Cleanup of the service operation description | 17.6.0
2022-09 | CT#97e | CP-222021 | 0137 | 1 | B | Subscribe to All TAIs for AMF Set | 18.0.0
2022-09 | CT#97e | CP-222025 | 0142 |  | F | 29.531 Rel-18 API version and External doc update | 18.0.0
2022-12 | CT#98e | CP-223028 | 0143 | 1 | F | Missing mandatory status codes in OpenAPI | 18.1.0
2022-12 | CT#98e | CP-223033 | 0144 | - | F | 29.531 Rel-18 API version and External doc update | 18.1.0
2023-03 | CT#99 | CP-230072 | 0146 | 1 | A | Essential correction of MNC encoding targetAmfSet | 18.2.0
2023-03 | CT#99 | CP-230071 | 0148 | - | F | 29.531 Rel-18 API version and External doc update | 18.2.0
2023-06 | CT#100 | CP-231028 | 0147 | 3 | F | Location header description | 18.3.0
2023-06 | CT#100 | CP-231027 | 0151 | 1 | F | Clarify the inclusion of targetAmfSet IE and candidateAmfList IE | 18.3.0
2023-06 | CT#100 | CP-231025 | 0152 | - | F | Clarify the inclusion of ueSupNssrgInd IE | 18.3.0
2023-06 | CT#100 | CP-231027 | 0153 | 1 | F | Clarify the content of mappingOfNssai IE | 18.3.0
2023-06 | CT#100 | CP-231025 | 0154 | - | F | Update data types re-used by Nnssf_NSSelection service | 18.3.0
2023-06 | CT#100 | CP-231048 | 0155 | 1 | B | Discontinuity of NSI for PDU sessions | 18.3.0
2023-06 | CT#100 | CP-231048 | 0156 | 5 | B | Support of Alternative S-NSSAI | 18.3.0
2023-06 | CT#100 | CP-231069 | 0159 | 1 | F | Editorial corrections | 18.3.0
2023-06 | CT#100 | CP-231028 | 0160 | - | F | Removal of apiVersion from resource URI variables tables | 18.3.0
2023-06 | CT#100 | CP-231070 | 0161 | - | F | 29.531 Rel-18 API version and External doc update | 18.3.0
2023-09 | CT#101 | CP-232069 | 0163 | 3 | B | Updates on Subscription, Unsubscription and Notification of NSSF for Network Slice and Network Slice Instance replacement | 18.4.0
2023-09 | CT#101 | CP-232043 | 0164 | 1 | B | Nnssf_NSSAIAvailability Service updata for Network Slice Replacement | 18.4.0
2023-09 | CT#101 | CP-232043 | 0165 | 1 | B | Remove the Editor's NOTE on Network Slice instance Replacement | 18.4.0
2023-09 | CT#101 | CP-232058 | 0166 | 1 | F | Major API version | 18.4.0
2023-09 | CT#101 | CP-232060 | 0167 | - | F | 29.531 Rel-18 API version and External doc update | 18.4.0
2023-12 | CT#102 | CP-233027 | 0168 | - | F | HTTP RFCs obsoleted by IETF RFC 9110 and 9113 | 18.5.0
2023-12 | CT#102 | CP-233044 | 0169 | - | F | Correction on the description of DateTime | 18.5.0
2023-12 | CT#102 | CP-233071 | 0172 | 2 | A | Update of subscribed NSSAI when UE is not registered in network | 18.5.0
2023-12 | CT#102 | CP-233058 | 0175 | 3 | A | Correction of API TS29531_Nnssf_NSSAIAvailability.yaml, error in amfSetId pattern | 18.5.0
2023-12 | CT#102 | CP-233030 | 0176 | - | F | ProblemDetails RFC 7807 obsoleted by 9457 | 18.5.0
2023-12 | CT#102 | CP-233060 | 0180 | - | F | 29.531 Rel-18 API version and External doc update | 18.5.0
2024-03 | CT#103 | CP-240053 | 0185 | 2 | F | Presence Condition of Default Configured S-NSSAI Indication | 18.6.0
2024-03 | CT#103 | CP-240071 | 0187 | 1 | A | NSSF determination of Allowed NSSAI | 18.6.0
2024-03 | CT#103 | CP-240042 | 0188 | 1 | B | Clarification on Congestion Mitigation Information during NSSAIAvailability Notification for Network Slice Replacement | 18.6.0
2024-03 | CT#103 | CP-240042 | 0191 | 1 | F | Support Network Slice Replacement and Network Slice Instance Replacement in roaming | 18.6.0
2024-03 | CT#103 | CP-240042 | 0192 | 1 | B | Service operation update for S-NSSAI validity time | 18.6.0
2024-03 | CT#103 | CP-240042 | 0193 | 1 | B | Data type definition for S-NSSAI validity time | 18.6.0
2024-03 | CT#103 | CP-240053 | 0194 | - | F | Editorial and Style Corrections | 18.6.0
2024-03 | CT#103 | CP-240042 | 0196 | - | F | Editorial corrections | 18.6.0
2024-03 | CT#103 | CP-240056 | 0197 | - | F | 29.531 Rel-18 API version and External doc update | 18.6.0
2024-06 | CT#104 | CP-241028 | 0199 | 1 | F | Callbacks | 18.7.0
2024-06 | CT#104 | CP-241050 | 0201 | 1 | F | Miscellaneous corrections | 18.7.0
2024-06 | CT#104 | CP-241028 | 0202 | 1 | B | Returning UNSUPPORTED_EVENT_TYPE | 18.7.0
2024-06 | CT#104 | CP-241050 | 0203 | - | F | Feature negotiation correction | 18.7.0
2024-06 | CT#104 | CP-241059 | 0205 | 1 | A | NWDAF as consumer of NSSF service | 18.7.0
2024-06 | CT#104 | CP-241066 | 0208 | 1 | A | SMF as consumer of NSSF | 18.7.0
2024-06 | CT#104 | CP-241052 | 0209 | - | F | 29.531 Rel-18 API version and External doc update | 18.7.0
2024-09 | CT#105 | CP-242053 | 0216 | 1 | F | Miscellaneous corrections | 18.8.0
2024-09 | CT#105 | CP-242067 | 0219 | 1 | A | Corrections on API version in URI of the service operations | 18.8.0
2024-09 | CT#105 | CP-242035 | 0213 | 1 | B | Indirect Network Sharing Deployment Support | 19.0.0
2024-09 | CT#105 | CP-242033 | 0217 | 1 | F | Correction on subscriptionId in notification message | 19.0.0
2024-12 | CT#106 | CP-243032 | 0221 | - | F | Correction on NSAG Info List attribute | 19.1.0
2024-12 | CT#106 | CP-243037 | 0222 | 1 | B | Support of Indirect Network Sharing | 19.1.0
2024-12 | CT#106 | CP-243037 | 0223 | 3 | B | Resolve EN for Network Slice Selection in Participating Operator Network | 19.1.0
2024-12 | CT#106 | CP-243059 | 0228 | - | F | Correction on enumerations | 19.1.0
2025-03 | CT#107 | CP-250035 | 0231 | 1 | B | Correction on the description of the attribute of sNssaiForMapping | 19.2.0
2025-06 | CT#108 | CP-251067 | 0232 | 1 | D | Editorial Correction | 19.3.0
2025-06 | CT#108 | CP-251052 | 0234 | 2 | A | Essential Correction on SNSSAI Validity Time Information | 19.3.0
2025-06 | CT#108 | CP-251047 | 0236 | - | A | Correction on IE Conditions | 19.3.0
2025-06 | CT#108 | CP-251067 | 0239 | 1 | F | Corrects the OpenAPI definition of nfId in URI of NSSAI Availability resource | 19.3.0
2025-06 | CT#108 | CP-251067 | 0240 | 1 | C | Allowing discovery of NSSF via NRF | 19.3.0
2025-06 | CT#108 | CP-251067 | 0241 | 1 | B | Partial Successful Subscription Creation | 19.3.0
2025-06 | CT#108 | CP-251079 | 0243 | - | F | 29.531 Rel-19 API version and External doc update | 19.3.0
2025-09 | CT#109 | CP-252049 | 0244 | 3 | F | Text correction for Nnssf_NSSAIAvailability Service | 19.4.0
2025-09 | CT#109 | CP-252049 | 0248 | 1 | D | Clarification for NssfEventType | 19.4.0
2025-09 | CT#109 | CP-252176 | 0252 | - | F | 29.531 Rel-19 API version and External doc update | 19.4.0
2025-12 | CT#110 | CP-253153 | 0253 | - | F | Correction of attribute presence | 19.5.0
2025-12 | CT#110 | CP-253167 | 0254 | - | F | 29.531 Rel-19 API version and External doc update | 19.5.0
2026-03 | CT#111 | CP-260030 | 0256 | - | F | Clarification of Configured NSSAI mapping and Rejected NSSAI handling | 19.6.0
