---
source_spec: "29.531"
source_path: specs/29.531/29531-j60.docx
source_mtime: 1778311321.968539
section: "6.1"
title: "Nnssf_NSSelection Service API"
generator: spec-split.py
generator_version: 1
chars: 9844
---

# Nnssf_NSSelection Service API

6.1	Nnssf_NSSelection Service API
6.1.1	API URI
The Nnssf_NSSelection service shall use the Nnssf_NSSelection API.
The API URI of the Nnssf_NSSelection API shall be:
{apiRoot}/<apiName>/<apiVersion>
The request URIs used in HTTP requests from the NF service consumer towards the NF service producer shall have the Resource URI structure defined in clause 4.4.1 of 3GPP TS 29.501 [5], i.e.:
{apiRoot}/<apiName>/<apiVersion>/<apiSpecificResourceUriPart>
with the following components:
-	The {apiRoot} shall be set as described in 3GPP TS 29.501 [5].
-	The <apiName> shall be "nnssf-nsselection".
-	The <apiVersion> shall be "v2".
-	The <apiSpecificResourceUriPart> shall be set as described in clause 6.1.3.
6.1.2	Usage of HTTP
6.1.2.1	General
HTTP/2, IETF RFC 9113 [10], shall be used as specified in clause 5 of 3GPP TS 29.500 [4].
An OpenAPI [6] specification of HTTP messages and content bodies for the Nnssf_NSSelection service is specified in Annex A.
6.1.2.2	HTTP standard headers
6.1.2.2.1	General
See clause 5.2.2 of 3GPP TS 29.500 [4] for the usage of HTTP standard headers.
6.1.2.2.2	Content type
 The following content types shall be supported:
-	JSON, as defined in IETF RFC 8259 [14], shall be used as content type of the HTTP bodies specified in the present specification as indicated in clause 5.4 of 3GPP TS 29.500 [4].
-	The Problem Details JSON Object (IETF RFC 9457 [15]. The use of the Problem Details JSON object in a HTTP response body shall be signalled by the content type "application/problem+json".
6.1.2.3	HTTP custom headers
6.1.2.3.1	General
In this release of this specification, no custom headers specific to the Nnssf_NSSelection service are defined. For 3GPP specific HTTP custom headers used across all service based interfaces, see clause 5.2.3 of 3GPP TS 29.500 [4].
6.1.3	Resources
6.1.3.1	Overview
This clause describes the structure for the Resource URIs and the resources and methods used for the service.
Figure 6.1.3.1-1 describes the resource URI structure of the Nnssf_NSSelection API.
Figure 6.1.3.1-1: Resource URI structure of the nnssf_nsselection API
Table 6.1.3.1-1 provides an overview of the resources and applicable HTTP methods.
Table 6.1.3.1-1: Resources and methods overview
6.1.3.2	Resource:  Network Slice Information
6.1.3.2.1	Description
This resource represents the network slice related information maintained by the NSSF.This resource is modelled with the Document resource archetype (see clause C.1 of 3GPP TS 29.501 [5]).
6.1.3.2.2	Resource Definition
Resource URI: {apiRoot}/nnssf-nsselection/<apiVersion>/network-slice-information
This resource shall support the resource URI variables defined in table 6.1.3.2.2-1.
Table 6.1.3.2.2-1: Resource URI variables for this resource
6.1.3.2.3	Resource Standard Methods
6.1.3.2.3.1	GET
This method retrieves the information related to the selected slice based on the input query parameters provided by the NF service consumer specified in table 6.1.3.2.3.1-1.
This method shall support input query parameters specified in table 6.1.3.2.3.1-1 and the response data structure and response codes specified in table 6.1.3.2.3.1-3.
Table 6.1.3.2.3.1-1: URI query parameters supported by the GET method on this resource
Table 6.1.3.2.3.1-2: Data structures supported by the GET Request Body on this resource
Table 6.1.3.2.3.1-3: Data structures supported by the GET Response Body on this resource
Table 6.1.3.2.3.1-4: Headers supported by the 307 Response Code on this resource
Table 6.1.3.2.3.1-5: Headers supported by the 308 Response Code on this resource
6.1.3.2.4	Resource Custom Operations
There are no custom methods supported on the network-slice-information collection resource.
6.1.4	Custom Operations without associated resources
There are no custom operations without associated resources defined for the Nnssf_NSSelection service in this version of this API.
6.1.5	Notifications
In this release of this specification, there are no notifications defined for the Nnssf_NSSelection service.
6.1.6	Data Model
6.1.6.1	General
This clause specifies the application data model supported by the API.
Table 6.1.6.1-1 specifies the data types defined for the Nnssf_NSSelection service based interface protocol.
Table 6.1.6.1-1: Nnssf_NSSelection specific Data Types
Table 6.1.6.1-2 specifies data types re-used by the Nnssf_NSSelection service based interface protocol from other specifications, including a reference to their respective specifications and when needed, a short description of their use within the Nnssf_NSSelection service based interface.
Table 6.1.6.1-2: Nnssf_NSSelection re-used Data Types
6.1.6.2	Structured data types
6.1.6.2.1	Introduction
This clause defines the structures to be used in resource representations.
6.1.6.2.2	Type: AuthorizedNetworkSliceInfo
Table 6.1.6.2.2-1: Definition of type AuthorizedNetworkSliceInfo
6.1.6.2.3	Type: SubscribedSnssai
Table 6.1.6.2.3-1: Definition of type SubscribedSnssai
6.1.6.2.4	Void
6.1.6.2.5	Type: AllowedSnssai
Table 6.1.6.2.5-1: Definition of type AllowedSnssai
6.1.6.2.6	Type: AllowedNssai
Table 6.1.6.2.6-1: Definition of type AllowedNssai
6.1.6.2.7	Type: NsiInformation
Table 6.1.6.2.7-1: Definition of type NsiInformation
6.1.6.2.8	Type: MappingOfSnssai
Table 6.1.6.2.8-1: Definition of type MappingOfSnssai
6.1.6.2.9	Void
6.1.6.2.10	Type: SliceInfoForRegistration
Table 6.1.6.2.10-1: Definition of type SliceInfoForRegistration
6.1.6.2.11	Type: SliceInfoForPDUSession
Table 6.1.6.2.11-1: Definition of type SliceInfoForPDUSession
6.1.6.2.12	Type: ConfiguredSnssai
Table 6.1.6.2.12-1: Definition of type ConfiguredSnssai
6.1.6.2.13	Type: SliceInfoForUEConfigurationUpdate
Table 6.1.6.2.13-1: Definition of type SliceInfoForUEConfigurationUpdate
6.1.6.2.14	Type: NsagInfo
Table 6.1.6.2.14-1: Definition of type NsagInfo
6.1.6.2.15	Type: SnssaiInfo
Table 6.1.6.2.15-1: Definition of type SnssaiInfo
6.1.6.3	Simple data types and enumerations
6.1.6.3.1	Introduction
This clause defines simple data types and enumerations that can be referenced from data structures defined in the previous clauses.
6.1.6.3.2	Simple data types
The simple data types defined in table 6.1.6.3.2-1 shall be supported.
Table 6.1.6.3.2-1: Simple data types
6.1.6.3.3	Enumeration: RoamingIndication
Table 6.1.6.3.3-1: Enumeration RoamingIndication
6.1.6.4	Binary data
There is no binary data used for the Nnssf_NSSelection service in this version of the API.
6.1.7	Error Handling
6.1.7.1	General
HTTP error handling shall be supported as specified in clause 5.2.4 of 3GPP TS 29.500 [4].
6.1.7.2	Protocol Errors
Protocol Error Handling shall be supported as specified in clause 5.2.7.2 of 3GPP TS 29.500 [4].
6.1.7.3	Application Errors
The common application errors defined in the Table 5.2.7.2-1 in 3GPP TS 29.500 [4] may also be used for the Nnssf_NSSelection service. The following application errors listed in Table 6.1.7.3-1 are specific for the Nnssf_NSSelection service.
Table 6.1.7.3-1: Application errors
6.1.8	Feature negotiation
The feature negotiation mechanism specified in clause 6.6 of 3GPP TS 29.500 [4] shall be used to negotiate the features applicable between the NSSF and the NF Service Consumer, for the Nnssf_NSSelection service, if any.
The NF Service Consumer shall indicate the features it supports for the Nnssf_NSSelection service, if any, by including the supportedFeatures attribute in the HTTP GET request when requesting the NSSF to provide the Allowed NSSAI information.
The NSSF shall determine the supported features for the requested network slice information resource as specified in clause 6.6 of 3GPP TS 29.500 [4] and shall indicate the supported features by including the supportedFeatures attribute in the Allowed NSSAI information it returns in the HTTP response.
The syntax of the supportedFeatures attribute is defined in clause 5.2.2 of 3GPP TS 29.571 [7].
The following features are defined for the Nnssf_NSSelection service.
Table 6.1.8-1: Features of supportedFeatures attribute used by Nnssf_NSSelection service
6.1.9	Security
As indicated in 3GPP TS 33.501 [11] and 3GPP TS 29.500 [4], the access to the Nnssf_NSSelection API may be authorized by means of the OAuth2 protocol (see IETF RFC 6749 [12]), based on local configuration, using the "Client Credentials" authorization grant, where the NRF (see 3GPP TS 29.510 [13]) plays the role of the authorization server.
If OAuth2 is used, an NF Service Consumer, prior to consuming services offered by the Nnssf_NSSelection API, shall obtain a "token" from the authorization server, by invoking the Access Token Request service, as described in 3GPP TS 29.510 [13], clause 5.4.2.2.
NOTE:	When multiple NRFs are deployed in a network, the NRF used as authorization server is the same NRF that the NF Service Consumer used for discovering the Nnssf_NSSelection service.
The Nnssf_NSSelection API does not define any scopes for OAuth2 authorization.
6.1.10	HTTP redirection
An HTTP request may be redirected to a different NSSF service instance, within the same NSSF or a different NSSF of an NSSF set, e.g. when an NSSF service instance is part of an NSSF (service) set or when using indirect communications (see 3GPP TS 29.500 [4]). See the ES3XX feature in clause 6.1.8.
An SCP that reselects a different NSSF producer instance will return the NF Instance ID of the new NSSF producer instance in the 3gpp-Sbi-Producer-Id header, as specified in clause 6.10.3.4 of 3GPP TS 29.500 [4].
If an NSSF within an NSSF set redirects a service request to a different NSSF of the set using a 307 Temporary Redirect or 308 Permanent Redirect status code, the identity of the new NSSF towards which the service request is redirected shall be indicated in the 3gpp-Sbi-Target-Nf-Id header of the 307 Temporary Redirect or 308 Permanent Redirect response as specified in clause 6.10.9.1 of 3GPP TS 29.500 [4].
