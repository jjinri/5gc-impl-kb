---
source_spec: "29.531"
source_path: specs/29.531/29531-j60.docx
source_mtime: 1778311321.968539
section: "6.2"
title: "Nnssf_NSSAIAvailability Service API"
generator: spec-split.py
generator_version: 1
chars: 17040
---

# Nnssf_NSSAIAvailability Service API

6.2	Nnssf_NSSAIAvailability Service API
6.2.1	API URI
The Nnssf_NSSAIAvailability service shall use the Nnssf_ NSSAIAvailability API.
The API URI of the Nnssf_NSSAIAvailability API shall be:
{apiRoot}/nnssf- nssaiavailability/<apiVersion>
The request URIs used in HTTP requests from the NF service consumer towards the NF service producer shall have the Resource URI structure defined in clause 4.4.1 of 3GPP TS 29.501 [5], i.e.:
{apiRoot}/nnssf- nssaiavailability/<apiVersion>/<apiSpecificResourceUriPart>
with the following components:
-	The {apiRoot} shall be set as described in 3GPP TS 29.501 [5].
-	The <apiVersion> shall be "v1".
-	The <apiSpecificResourceUriPart> shall be set as described in clause 6.2.3.
6.2.2	Usage of HTTP
6.2.2.1	General
HTTP/2, IETF RFC 9113 [10], shall be used as specified in clause 5 of 3GPP TS 29.500 [4].
An OpenAPI [6] specification of HTTP messages and content bodies for the Nnssf_NSSAIAvailability service is specified in Annex A.
6.2.2.2	HTTP standard headers
6.2.2.2.1	General
See clause 5.2.2 of 3GPP TS 29.500 [4] for the usage of HTTP standard headers.
6.2.2.2.2	Content type
The JSON format shall be supported. The use of JSON format shall be as specified in clause 5.4 of 3GPP TS 29.500 [4].
The following content types shall be supported:
-	JSON, as defined in IETF RFC 8259 [14], shall be used as content type of the HTTP bodies specified in the present specification as indicated in clause 5.4 of 3GPP TS 29.500 [4].
-	The Problem Details JSON Object (IETF RFC 9457 [15]). The use of the Problem Details JSON object in a HTTP response body shall be signalled by the content type "application/problem+json".
-	JSON Patch (IETF RFC 6902 [8]). The use of the JSON Patch format in a HTTP request body shall be signalled by the content type "application/json-patch+json".
6.2.2.2.3	Accept-Encoding
The NSSF should support gzip coding (see IETF RFC 1952 [16]) in HTTP requests and responses and indicate so in the Accept-Encoding header, as described in clause 6.9 of 3GPP TS 29.500 [4].
6.2.2.3	HTTP custom headers
6.2.2.3.1	General
In this release of this specification, no custom headers specific to the Nnssf_NSSAIAvailability service are defined. For 3GPP specific HTTP custom headers used across all service based interfaces, see clause 5.2.3 of 3GPP TS 29.500 [4].
6.2.3	Resources
6.2.3.1	Overview
This clause describes the structure for the Resource URIs and the resources and methods used for the service.
Figure 6.2.3.1-1 describes the resource URI structure of the Nnssf_NSSAIAvailability API.
Figure 6.2.3.1-1: Resource URI structure of the Nnssf_NSSAIAvailability API
Table 6.2.3.1-1 provides an overview of the resources and applicable HTTP methods.
Table 6.2.3.1-1: Resources and methods overview
6.2.3.2	Resource: NSSAI Availability Document
6.2.3.2.1	Description
This resource represents a single  NSSAI Availability resource.
This resource is modelled with the Document resource archetype (see clause C.1 of 3GPP TS 29.501 [5]).
6.2.3.2.2	Resource Definition
Resource URI: {apiRoot}/nnssf-nssaiavailability/<apiVersion>/nssai-availability/{nfId}
This resource shall support the resource URI variables defined in table 6.2.3.2.2-1.
Table 6.2.3.2.2-1: Resource URI variables for this resource
6.2.3.2.3	Resource Standard Methods
6.2.3.2.3.1	PUT
This method shall support the request data structures specified in table 6.2.3.2.3.1-1 and the response data structures and response codes specified in table 6.2.3.2.3.1-2.
Table 6.2.3.2.3.1-1: Data structures supported by the PUT Request Body on this resource
Table 6.2.3.2.3.1-2: Data structures supported by the PUT Response Body on this resource
Table 6.2.3.2.3.1-3: Headers supported by the PUT method on this resource
Table 6.2.3.2.3.1-4: Headers supported by the 200 Response Code on this resource
Table 6.2.3.2.3.1-5: Headers supported by the 307 Response Code on this resource
Table 6.2.3.2.3.1-6: Headers supported by the 308 Response Code on this resource
6.2.3.2.3.2	PATCH
This method shall support the request data structures specified in table 6.2.3.2.3.2-1 and the response data structures and response codes specified in table 6.2.3.2.3.2-2.
Table 6.2.3.2.3.2-1: Data structures supported by the PATCH Request Body on this resource
Table 6.2.3.2.3.2-2: Data structures supported by the PATCH Response Body on this resource
Table 6.2.3.2.3.2-3: Headers supported by the 307 Response Code on this resource
Table 6.2.3.2.3.2-4: Headers supported by the 308 Response Code on this resource
6.2.3.2.3.3	DELETE
This method shall support the request data structures specified in table 6.2.3.2.3.3-1 and the response data structures and response codes specified in table 6.2.3.2.3.3-2.
Table 6.2.3.2.3.3-1: Data structures supported by the DELETE Request Body on this resource
Table 6.2.3.2.3.3-2: Data structures supported by the DELETE Response Body on this resource
Table 6.2.3.2.3.3-3: Headers supported by the 307 Response Code on this resource
Table 6.2.3.2.3.3-4: Headers supported by the 308 Response Code on this resource
6.2.3.3	Resource: NSSAI Availability Notification Subscriptions Collection
6.2.3.3.1	Description
This resource represents the collection of NSSAI Availability Notification Subscriptions in the NSSF.
This resource is modelled with the Collection resource archetype (see clause C.2 of 3GPP TS 29.501 [5]).
6.2.3.3.2	Resource Definition
Resource URI: {apiRoot}/nnssf-nssaiavailability/<apiVersion>/nssai-availability/subscriptions
This resource shall support the resource URI variables defined in table 6.2.3.3.2-1.
Table 6.2.3.3.2-1: Resource URI variables for this resource
6.2.3.3.3	Resource Standard Methods
6.2.3.3.3.1	POST
This method shall support the request data structures specified in table 6.2.3.3.3.1-1 and the response data structures and response codes specified in table 6.2.3.3.3.1-2.
Table 6.2.3.3.3.1-1: Data structures supported by the POST Request Body on this resource
Table 6.2.3.3.3.1-2: Data structures supported by the POST Response Body on this resource
Table 6.2.3.3.3.1-3: Headers supported by the 201 Response Code on this resource
Table 6.2.3.3.3.1-4: Headers supported by the 307 Response Code on this resource
Table 6.2.3.3.3.1-5: Headers supported by the 308 Response Code on this resource
6.2.3.4	Resource: Individual NSSAI Availability Notification Subscriptions
6.2.3.4.1	Description
This resource represents an Individual NSSAI Availability Notification Subscriptions resources generated by the NSSF.
This resource is modelled with the Document resource archetype (see clause C.1 of 3GPP TS 29.501 [5]).
6.2.3.4.2	Resource Definition
Resource URI: {apiRoot}/nnssf-nssaiavailability/<apiVersion>/nssai-availability/subscriptions/{subscriptionId}
This resource shall support the resource URI variables defined in table 6.2.3.4.2-1.
Table 6.2.3.4.2-1: Resource URI variables for this resource
6.2.3.4.3	Resource Standard Methods
6.2.3.4.3.1	DELETE
This method shall support the request data structures specified in table 6.2.3.4.3.1-1 and the response data structures and response codes specified in table 6.2.3.4.3.1-2.
Table 6.2.3.4.3.1-1: Data structures supported by the DELETE Request Body on this resource
Table 6.2.3.4.3.1-2: Data structures supported by the DELETE Response Body on this resource
Table 6.2.3.4.3.1-3: Headers supported by the 307 Response Code on this resource
Table 6.2.3.4.3.1-4: Headers supported by the 308 Response Code on this resource
6.2.3.4.3.2	PATCH
This method shall support the request data structures specified in table 6.2.3.4.3.2-1 and the response data structures and response codes specified in table 6.2.3.4.3.2-2.
Table 6.2.3.4.3.2-1: Data structures supported by the PATCH Request Body on this resource
Table 6.2.3.4.3.2-2: Data structures supported by the PATCH Response Body on this resource
Table 6.2.3.4.3.2-3: Headers supported by the 307 Response Code on this resource
Table 6.2.3.4.3.2-4: Headers supported by the 308 Response Code on this resource
6.2.3.5	Resource: NSSAI Availability Store
6.2.3.5.1	Description
This resource represents a collection of NSSAI Availability resources.
This resource is modelled with the Store resource archetype (see clause C.1 of 3GPP TS 29.501 [5]).
6.2.3.5.2	Resource Definition
Resource URI: {apiRoot}/nnssf-nssaiavailability/<apiVersion>/nssai-availability
This resource shall support the resource URI variables defined in table 6.2.3.5.2-1.
Table 6.2.3.5.2-1: Resource URI variables for this resource
6.2.3.5.3	Resource Standard Methods
6.2.3.5.3.1	OPTIONS
This method queries the communication options supported by the NSSF (see clause 6.9 of 3GPP TS 29.500 [4]). This method shall support the URI query parameters specified in table 6.1.3.5.3.1-1.
Table 6.2.3.5.3.1-1: URI query parameters supported by the OPTIONS method on this resource
This method shall support the request data structures specified in table 6.2.3.5.3.1-2 and the response data structures and response codes specified in table 6.2.3.5.3.2-3.
Table 6.2.3.5.3.1-2: Data structures supported by the OPTIONS Request Body on this resource
Table 6.2.3.5.3.1-3: Data structures supported by the OPTIONS Response Body on this resource
Table 6.2.3.5.3.1-4: Headers supported by the 200 Response Code on this resource
Table 6.2.3.5.3.1-5: Headers supported by the 307 Response Code on this resource
Table 6.2.3.5.3.1-6: Headers supported by the 308 Response Code on this resource
6.2.4	Custom Operations without associated resources
There are no custom operations without associated resources for the Nnssf_NSSAIAvailability service in this version of the API.
6.2.5	Notifications
6.2.5.1	General
This clause specifies the notifications provided by the Nnssf_NSSAIAvailability service.
Table 6.2.5.1-1: Notifications overview
6.2.5.2	NSSAI Availability Notification
6.2.5.2.1	Description
If the NF Service Consumer (e.g. AMF or V-NSSF) has provided the callback URI for getting notified about the NSSAI availability information, Network Slice Replacement or Network Slice Instance Replacement, the NSSF shall notify the NF Service Consumer whenever the NSSAI availability information, Network Slice Replacement or Network Slice Instance Replacement is updated.
6.2.5.2.2	Notification Definition
Callback URI: {nfNssaiAvailabilityUri}
This callback URI is provided by the NF Service Consumer (e.g. AMF or V-NSSF) during subscription creation invoked by the NF Service Consumer.
Table 6.2.5.2.2-1: Resources and methods overview
6.2.5.2.3	Notification Standard Methods
6.2.5.2.3.1	POST
This method shall support the request data structures specified in table 6.2.5.2.3.1-1 and the response data structures and response codes specified in table 6.2.5.2.3.1-2.
Table 6.2.5.2.3.1-1: Data structures supported by the POST Request Body on this resource
Table 6.2.5.2.3.1-2: Data structures supported by the POST Response Body on this resource
Table 6.2.5.2.3.1-3: Headers supported by the 307 Response Code on this resource
Table 6.2.5.2.3.1-4: Headers supported by the 308 Response Code on this resource
6.2.6	Data Model
6.2.6.1	General
This clause specifies the application data model supported by the API.
Table 6.2.6.1-1 specifies the data types defined for the Nnssf_NSSAIAvailability service based interface protocol.
Table 6.2.6.1-1: Nnssf_NSSAIAvailability specific Data Types
Table 6.2.6.1-2 specifies data types re-used by the Nnssf_NSSAIAvailability service based interface protocol from other specifications, including a reference to their respective specifications and when needed, a short description of their use within the Nnssf_NSSAIAvailability service based interface.
Table 6.2.6.1-2: Nnssf_NSSAIAvailability re-used Data Types
6.2.6.2	Structured data types
6.2.6.2.1	Introduction
This clause defines the structures to be used in resource representations.
6.2.6.2.2	Type: NssaiAvailabilityInfo
Table 6.2.6.2.2-1: Definition of type NssaiAvailabilityInfo
6.2.6.2.3	Type: SupportedNssaiAvailabilityData
Table 6.2.6.2.3-1: Definition of type SupportedNssaiAvailabilityData
6.2.6.2.4	Type: AuthorizedNssaiAvailabilityData
Table 6.2.6.2.4-1: Definition of type AuthorizedNssaiAvailabilityData
6.2.6.2.5	Type: RestrictedSnssai
Table 6.2.6.2.5-1: Definition of type RestrictedSnssai
6.2.6.2.6	Type: AuthorizedNssaiAvailabilityInfo
Table 6.2.6.2.6 -1: Definition of type AuthorizedNssaiAvailabilityInfo
6.2.6.2.7	Type: PatchDocument
Table 6.2.6.2.7-1: Definition of type PatchDocument
6.2.6.2.8	Type: NssfEventSubscriptionCreateData
Table 6.2.6.2.8-1: Definition of type NssfEventSubscriptionCreateData
6.2.6.2.9	Type: NssfEventSubscriptionCreatedData
Table 6.2.6.2.9-1: Definition of type NssfEventSubscriptionCreatedData
6.2.6.2.10	Type: NssfEventNotification
Table 6.2.6.2.10-1: Definition of type NssfEventNotification
6.2.6.2.11	Type: SnssaiReplacementSubscribeInfo
Table 6.2.6.2.11-1: Definition of type SnssaiReplacementSubscribeInfo
6.2.6.2.12	Type: NsiUnavailabilitySubscribeInfo
Table 6.2.6.2.12-1: Definition of type NsiUnavailabilitySubscribeInfo
6.2.6.3	Simple data types and enumerations
6.2.6.3.1	Introduction
This clause defines simple data types and enumerations that can be referenced from data structures defined in the previous clauses.
6.2.6.3.2	Simple data types
The simple data types defined in table 6.2.6.3.2-1 shall be supported.
Table 6.2.6.3.2-1: Simple data types
6.2.6.3.3	Enumeration: NssfEventType
Table 6.2.6.3.3-1: Enumeration NssfEventType
6.2.6.4	Binary data
There is no binary data used for the Nnssf_NSSAIAvailability service in this version of the API.
6.2.7	Error Handling
6.2.7.1	General
HTTP error handling shall be supported as specified in clause 5.2.4 of 3GPP TS 29.500 [4].
6.2.7.2	Protocol Errors
Protocol Error Handling shall be supported as specified in clause 5.2.7.2 of 3GPP TS 29.500 [4].
6.2.7.3	Application Errors
The common application errors defined in the Table 5.2.7.2-1 in 3GPP TS 29.500 [4] may also be used for the Nnssf_NSSAIAvailability service. The following application errors listed in Table 6.1.7.3-1 are specific for the Nnssf_NSSAIAvailability service.
Table 6.2.7.3-1: Application errors
6.2.8	Feature negotiation
The feature negotiation mechanism specified in clause 6.6 of 3GPP TS 29.500 [4] shall be used to negotiate the features applicable between the NSSF and the NF Service Consumer, for the Nnssf_NSSAIAvailability service, if any.
The NF Service Consumer shall indicate the features it supports for the Nnssf_NSSAIAvailability service, if any, by including the supportedFeatures attribute in the HTTP PUT request when requesting the NSSF to update the NSSAI Availability information.
The NSSF shall determine the supported features for the updated NSSAI Availability information resource as specified in clause 6.6 of 3GPP TS 29.500 [4] and shall indicate the supported features by including the supportedFeatures attribute in the authorized NSSAI availability information it returns in the HTTP response.
The syntax of the supportedFeatures attribute is defined in clause 5.2.2 of 3GPP TS 29.571 [7].
The following features are defined for the Nnssf_NSSAIAvailability service.
Table 6.2.8-1: Features of supportedFeatures attribute used by Nnssf_NSSAIAvailability service
6.2.9	Security
As indicated in 3GPP TS 33.501 [11] and 3GPP TS 29.500 [4], the access to the Nnssf_NSSAIAvailability API may be authorized by means of the OAuth2 protocol (see IETF RFC 6749 [12]), based on local configuration, using the "Client Credentials" authorization grant, where the NRF (see 3GPP TS 29.510 [13]) plays the role of the authorization server.
If OAuth2 is used, an NF Service Consumer, prior to consuming services offered by the Nnssf_NSSAIAvailability API, shall obtain a "token" from the authorization server, by invoking the Access Token Request service, as described in 3GPP TS 29.510 [13], clause 5.4.2.2.
NOTE:	When multiple NRFs are deployed in a network, the NRF used as authorization server is the same NRF that the NF Service Consumer used for discovering the Nnssf_NSSAIAvailability service.
The Nnssf_NSSAIAvailability API does not define any scopes for OAuth2 authorization.
6.2.10	HTTP redirection
An HTTP request may be redirected to a different NSSF service instance, within the same NSSF or a different NSSF of an NSSF set, e.g. when an NSSF service instance is part of an NSSF (service) set or when using indirect communications (see 3GPP TS 29.500 [4]). See the ES3XX feature in clause 6.2.8.
An SCP that reselects a different NSSF producer instance will return the NF Instance ID of the new NSSF producer instance in the 3gpp-Sbi-Producer-Id header, as specified in clause 6.10.3.4 of 3GPP TS 29.500 [4].
If an NSSF within an NSSF set redirects a service request to a different NSSF of the set using a 307 Temporary Redirect or 308 Permanent Redirect status code, the identity of the new NSSF towards which the service request is redirected shall be indicated in the 3gpp-Sbi-Target-Nf-Id header of the 307 Temporary Redirect or 308 Permanent Redirect response as specified in clause 6.10.9.1 of 3GPP TS 29.500 [4].
