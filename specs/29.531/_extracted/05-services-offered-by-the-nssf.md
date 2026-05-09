---
source_spec: "29.531"
source_path: specs/29.531/29531-j60.docx
source_mtime: 1778311321.968539
section: "5"
title: "Services offered by the NSSF"
generator: spec-split.py
generator_version: 1
chars: 32897
---

# Services offered by the NSSF

5	Services offered by the NSSF
5.1	Introduction
The NSSF supports the following services.
Table 5.1-1: NF Services provided by NSSF
Table 5.1-2 summarizes the corresponding APIs defined for this specification.
Table 5.1-2: API Descriptions
5.2	Nnssf_NSSelection Service
5.2.1	Service Description
The Nnssf_NSSelection service is used by an NF Service Consumer (e.g. AMF, SMF, NWDAF or NSSF in a different PLMN) to retrieve the information related to network slice in the non-roaming and roaming case.
It also enables the NSSF to provide to the AMF the Allowed NSSAI and the Configured NSSAI for the Serving PLMN.
It also enables the NSSF to provide to the AMF the NSAG information associated with the Configured NSSAI for the Serving PLMN.
It also enables the NSSF to provide to the SMF+PGW-C the mapping of S-NSSAI(s) of the VPLMN to corresponding HPLMN S-NSSAI(s).
It also enables the NSSF to provide to the NWDAF the NSI ID(s) associated with the requested S-NSSAI.
It also enables the NSSF to provide to the AMF the slice mapping information in the case of Indirect Network Sharing.
The NF service consumer may discover the NSSF in the same PLMN based on the local configuration, or by using the NRF-based discovery procedure as specified in clause 6.3.28 of 3GPP TS 23.501[2]. The addresses of the home NSSF may be either locally configured in the visited NSSF or discovered based on the self-constructed FQDN as specified in 3GPP TS 23.003 [9].
5.2.2	Service Operations
5.2.2.1	Introduction
For the Nnssf_NSSelection service the following service operations are defined:
-	Get.
5.2.2.2	GET
5.2.2.2.1	General
The Get operation shall be invoked by the AMF in the non-roaming or roaming scenario to retrieve:
-	The slice selection information including the Allowed NSSAI, Configured NSSAI, target AMF Set or the list of candidate AMF(s), and optionally
-	The Mapping Of Allowed NSSAI;
-	The Mapping Of Configured NSSAI;
-	NSI ID(s) associated with the Network Slice instances of the Allowed NSSAI;
-	NRF(s) to be used to select NFs/services within the selected Network Slice instance(s) and NRF to be used to determine the list of candidate AMF(s) from the AMF Set, during Registration procedure;
-	Information on whether the S-NSSAI(s) not included in the Allowed NSSAI which were part of the Requested NSSAI are rejected in the serving PLMN or in the current TA;
-	The Target NSSAI that includes the S-NSSAI(s) as defined in clause 5.3.4.3.3 of 3GPP TS 23.501 [2], and
-	The NSAG information associated with Configured NSSAI as defined in clause 5.15.14 of 3GPP TS 23.501 [2].
-	The NRF to be used to select NFs/services within the selected network slice instance, and optionally the NSI ID associated with the S-NSSAI provided in the input, during the PDU Session Establishment procedure.
-	The slice mapping information including the mapping of S-NSSAI(s) of the VPLMN to corresponding HPLMN S-NSSAI(s), which is also applicable to Indirect Network Sharing case.
The Get operation shall also be invoked by the vNSSF in the roaming scenario to retrieve:
-	The hNRF to be used to select NFs/services within the selected network slice instance in the HPLMN, and optionally the NSI ID associated with the S-NSSAI provided in the input, during the PDU Session Establishment procedure, which is also applicable to Indirect Network Sharing case.
The Get operation shall also be invoked by the SMF+PGW-C in VPLMN in the roaming scenario to retrieve:
-	The slice mapping information including the mapping of S-NSSAI(s) of the VPLMN to corresponding HPLMN S-NSSAI(s), during the PDN Connection Establishment procedure in EPC.
The Get operation shall also be invoked by the NWDAF to retrieve:
-	The NSI ID associated with the S-NSSAI provided in the input.
It is used in the following procedures:
-	Registration procedure (see clause 4.2.2.2.2 of 3GPP TS 23.502 [3]);
-	Registration with AMF re-allocation (see clause 4.2.2.2.3 of 3GPP TS 23.502 [3]);
-	EPS to 5GS handover using N26 interface (see clause 4.11.1.2.2 of 3GPP TS 23.502 [3]);
-	EPS to 5GS mobility registration procedure (see clauses 4.11.1.3.3, 4.11.1.3.3A, 4.11.1.3.4 and 4.23.12 of 3GPP TS 23.502 [3]);
-	Xn and N2 Handover procedures with PLMN change (see clauses 4.9.1, 4.23.7 and 4.23.11 of 3GPP TS 23.502 [3]);
-	UE Configuration Update procedure (see clause 4.2.4.2 of 3GPP TS 23.502 [3]);
-	SMF selection for non-roaming and roaming with local breakout (see clause 4.3.2.2.3.2 of 3GPP TS 23.502 [3]) or SMF selection for home-routed roaming scenario (see clause 4.3.2.2.3.3 of 3GPP TS 23.502 [3]);
-	PDN Connection Establishment (see clause 4.11.0a.5 of 3GPP TS 23.502 [3]);
-	Network Slice load analytics provided by NWDAF (see clause 6.3.4 of 3GPP TS 23.288 [22]).
NOTE:	The list of procedures above, which trigger invoking of the Nnssf_NSSelection_Get service operation, is not exhaustive.
5.2.2.2.2	Get service operation of Nnssf_NSSelection service
In this procedure, the NF Service Consumer (e.g. AMF) retrieves the slice selection information including the Allowed NSSAI, Configured NSSAI, target AMF Set or the list of candidate AMF(s) and other optional information.
This service operation shall also be used to retrieve the slice mapping information including the mapping of S-NSSAI(s) of the VPLMN to corresponding HPLMN S-NSSAI(s) e.g. during registration procedure of Indirect Network Sharing, inter-PLMN mobility procedure and/or mobility procedure within VPLMN from EPS to 5GS.
Figure 5.2.2.2.2-1: Retrieve the network slice information during the mobility procedure
1	The AMF shall send a GET request to the NSSF.
If the AMF wants to retrieve the slice selection information, one or more of the following parameters shall be included in the slice-info-request-for-registration query parameter:
-	Requested NSSAI and Subscribed S-NSSAI(s) with the indication if marked as default S-NSSAI and the associated subscribed NSSRG information;
-	optionally UE support of subscription-based restrictions to simultaneous registration of network slice feature Indication;
-	UDM indication to provide all subscribed S-NSSAIs for UEs not indicating support of subscription-based restrictions to simultaneous registration of network slices feature;
-	Indication of the support of NSAG by the UE.
If the AMF wants to retrieve the slice mapping information, the following parameters shall be included in the slice-info-request-for-registration query parameter:
-	sNssaiForMapping IE and;
-	requestMapping IE.
In both scenarios, the AMF shall also include the following parameters in the message:
-	PLMN ID of the SUPI in roaming scenarios or in the Indirect Network Sharing case;
-	TAI;
-	NF type of the NF service consumer and;
-	Requester ID.
2a	On success, "200 OK" shall be returned when the NSSF is able to find authorized network slice information for the requested network slice selection information, the response body shall include a content containing at least the following parameters:
-	Allowed NSSAI and;
-	target AMF Set or the list of candidate AMF(s).
The content may additionally contain the following parameters:
-	a target AMF Service Set;
-	Target NSSAI.
"200 OK" shall also be returned when the NSSF is able to find the requested slicing mapping information, the response body shall include a content containing the mapping of S-NSSAI(s) of the VPLMN to corresponding HPLMN S-NSSAI(s) included in the allowedNssaiList IE.
NSSFs of a PLMN that implement AMF reallocation via RAN by supporting the NGAP REROUTE NAS REQUEST procedure (see clause 8.6.5 of 3GPP TS 38.413 [21]) should return the target AMF set ID in its response. The NSSF may query the NRF to discover target AMF Set if this information is not known by other means (e.g. if not provided by AMF during Nnssf_NSSAIAvailability_Update service operation).
If subscribed NSSRG list is provided, the NSSF shall provide the compatible S-NSSAIs in the Allowed NSSAI as defined in clause 5.15.12 of 3GPP TS 23.501 [2] and compatible S-NSSAIs in the Target NSSAI (if provided).
If the request indicated that UE does not support subscription-based restrictions to simultaneous registration of network slice feature, and UDM has requested to provide all subscribed S-NSSAIs for such UEs, Configured NSSAI, if included, shall be provided ignoring the NSSRG restrictions.
If the AMF has indicated the support of NSAG by the UE, the NSSF shall include the "nsagInfos" attribute with NSAG information if available.
2b	If no slice instances can be found for the requested slice selection information or the requested slice mapping information, then the NSSF shall return a 403 Forbidden response with the "ProblemDetails" IE containing the Application Error "SNSSAI_NOT_SUPPORTED" (cf. Table 6.1.7.3-1).
On failure or redirection, the NSSF shall return one of the HTTP status codes together with the response body listed in Table 6.1.3.2.3.1-3.
5.2.2.2.3	Get service operation of Nnssf_NSSelection service during the PDU session establishment
In this procedure, the NF Service Consumer (e.g. AMF) retrieves the NRF and the optionally the NSI ID of the network slice instance:
Figure 5.2.2.2.3-1: Retrieve the network slice information during the PDU session establishment procedure
1	The NF Service consumer (e.g. AMF or NSSF in the different PLMN) shall send a GET request to the NSSF.
The request shall include query parameters, contain at least the following parameters:
-	S-NSSAI;
-	S-NSSAI from the HPLMN that maps to the S-NSSAI from the Allowed NSSAI of the Serving PLMN;
-	the NF type of the NF service consumer;
-	Requester ID and;
-	non-roaming/LBO roaming/HR roaming indication.
For the request towards an NSSF in the Serving PLMN, the query parameters shall also contain the PLMN ID of the SUPI and TAI.
2a	On success, "200 OK" shall be returned when the NSSF is able to find network slice instance information for the requested network slice selection information, the response body shall include a content containing at least the NRF to be used to select NFs/services within the selected Network Slice instance;
2b	If no slice instances can be found for the requested slice selection information, then the NSSF shall return a 403 Forbidden response with the "ProblemDetails" IE containing the Application Error "SNSSAI_NOT_SUPPORTED" (cf. Table 6.1.7.3-1).
On failure or redirection, the NSSF shall return one of the HTTP status codes together with the response body listed in Table 6.1.3.2.3.1-3.
5.2.2.2.4	Get service operation of Nnssf_NSSelection service during UE configuration update procedure
In this procedure, the NF Service Consumer (e.g. AMF) retrieves network slice configuration information (e.g. the Allowed NSSAI and the Configured NSSAI) during the UE configuration update procedure.
Figure 5.2.2.2.4-1: Retrieve the network slice information during UE configuration update procedure
1	The NF Service consumer (e.g. AMF) shall send a GET request to the NSSF. The request shall include query parameters:
-	Subscribed S-NSSAI(s) with the indication if the S-NSSAI is marked as default S-NSSAI and the associated subscribed NSSRG information;
-	optionally UE support of subscription-based restrictions to simultaneous registration of network slice feature Indication;
-	UDM indication to provide all subscribed S-NSSAIs for UEs not indicating support of subscription-based restrictions to simultaneous registration of network slices feature;
-	Rejected S-NSSAI(s) for the Registration Area;
-	PLMN ID of the SUPI;
-	TAI;
-	Indication of the support of NSAG by the UE;
-	NF type of the NF service consumer and;
-	the NF instance ID of the requester NF.
NOTE:	When the AMF invokes UE Configuration Update procedure to determine the Target NSSAI to redirect the UE to the dedicated frequency band(s) for an S-NSSAI (as specified in clause 5.3.4.3.3 of 3GPP TS 23.501 [2]), the AMF provides the Allowed NSSAI and the rejected S-NSSAI(s) for the current Registration Area to the NSSF; the Allowed NSSAI and Rejected S-NSSAI(s) for the RA does not include any S-NSSAI that failed for Network Slice-Specific Authentication and Authorization. The AMF does not include the Requested NSSAI to the NSSF in this procedure, thus the NSSF will not provide Allowed NSSAI again to the AMF in the response.
2a	On success, "200 OK" shall be returned when the NSSF is able to find authorized network slice information for the requested network slice selection information, the response body shall include a content containing at least the following parameters:
-	Allowed NSSAI;
-	Configured NSSAI and;
-	optionally Target NSSAI.
If subscribed NSSRG list is provided, the NSSF shall provide the compatible S-NSSAIs in the Allowed NSSAI as defined in the clause 5.15.12 of 3GPP TS 23.501 [2] and compatible S-NSSAIs in the Target NSSAI(if provided).
If the request indicated that UE does not support subscription-based restrictions to simultaneous registration of network slice feature, and UDM has requested to provide all subscribed S-NSSAIs for such UEs, the NSSF shall provide Configured NSSAI ignoring the NSSRG restrictions.
If the AMF has indicated the support of NSAG by the UE, the NSSF shall include the "nsagInfos" attribute with NSAG information if available.
2b	If no slice instances can be found for the requested slice selection information, then the NSSF shall return a 403 Forbidden response with the "ProblemDetails" IE containing the Application Error "SNSSAI_NOT_SUPPORTED" (cf. Table 6.1.7.3-1).
On failure or redirection, the NSSF shall return one of the HTTP status codes together with the response body listed in Table 6.1.3.2.3.1-3.
5.2.2.2.5	Get service operation of Nnssf_NSSelection service during the PDN Connection Establishment
In this procedure, the NF Service Consumer (e.g. SMF+PGW-C) retrieves the slice mapping information including the mapping of S-NSSAI(s) of the VPLMN to corresponding HPLMN S-NSSAI(s) from the NSSF that supports the RSIPCE feature, e.g. during PDN Connection Establishment procedure in EPC.
Figure 5.2.2.2.5-1: Retrieve the network slice information during the PDN Connection Establishment procedure
1	The NF Service consumer (e.g. SMF+PGW-C) shall send a GET request to the NSSF.
The request shall include query parameters slice-info-request-for-pdn-connection with a list of subscribed S-NSSAI(s);
The SMF+PGW-C shall also include the following parameters in the message:
-	PLMN ID of the SUPI;
-	the NF type of the NF service consumer and;
-	Requester ID.
2a	On success, "200 OK" shall be returned when the NSSF is able to find the requested slicing mapping information, the response body shall include a content containing the mapping of S-NSSAI(s) of the VPLMN to corresponding HPLMN S-NSSAI(s) included in the mappingOfNssai IE.
2b	If no slice instances can be found for the requested slicing mapping information, then the NSSF shall return a 403 Forbidden response with the "ProblemDetails" IE containing the Application Error "SNSSAI_NOT_SUPPORTED".
On failure or redirection, the NSSF shall return one of the HTTP status codes together with the response body listed in Table 6.1.3.2.3.1-3.
5.2.2.2.6	Get service operation of Nnssf_NSSelection service to retrieve the network slice information
The Get service operation shall be invoked by the NF Service Consumer (e.g. NWDAF) to retrieve the NSI ID of the network slice instance from the NSSF that supports the SIOP feature.
Figure 5.2.2.2.6-1: Retrieve the network slice information
1	The NF Service consumer (e.g. NWDAF) shall send a GET request to the NSSF. The request shall include query parameter slice-info-request-for-other-purpose includes the list of S-NSSAIs IE.
The NF Service consumer shall also include the following parameters in the message:
-	the NF type of the NF service consumer and;
-	Requester ID.
2a	On success, "200 OK" shall be returned when the NSSF is able to find slice information requested.
If the request includes the list of S-NSSAIs IE in query parameter slice-info-request-for-other-purpose from NWDAF, the response body shall include a content containing the NSI ID(s) for the requested S-NSSAIs.
2b	If no slice information can be found for the the list of S-NSSAIs in the request, then the NSSF shall return a 403 Forbidden response with the "ProblemDetails" IE containing the Application Error "SNSSAI_NOT_SUPPORTED".
On failure or redirection, the NSSF shall return one of the HTTP status codes together with the response body listed in Table 6.1.3.2.3.1-3.
5.3	Nnssf_NSSAIAvailability Service
5.3.1	Service Description
The Nnssf_NSSAIAvailability service is used by the NF service consumer (e.g AMF) to update the S-NSSAI(s) the AMF supports on a per TA basis on the NSSF, subscribe and unsubscribe the notification of any changes to the NSSAI availability information on a per TA basis, of the S-NSSAIs available per TA (unrestricted) and the restricted S-NSSAI(s) per PLMN in that TA in the serving PLMN of the UE.
It also enables the NF service consumer (e.g. AMF) to update the NSAG(s) associated with the S-NSSAI(s) supported by the AMF on a per TA basis.
It also enables the NF service consumer (e.g., AMF, V-NSSF) to receive updates for Network Slice Replacement and Network Slice Instance Replacement.
It also used by the NF service consumer (e.g. AMF) to subscribe and unsubscribe to the notification of any changes in the status of the NSSAI validity time information.
If the service operation is invoked for subscription to Network Slice Replacement and/or Network Slice Instance Replacement notification from AMF for roaming scenarios, then AMF shall subscribe to notification to the V-NSSF, which in turn shall subscribe to notification to the H-NSSF. If the event is triggered, the AMF receives the notification from V-NSSF and the V-NSSF receives the notification from H-NSSF.
5.3.2	Service Operations
5.3.2.1	Introduction
For the Nnssf_NSSAIAvailability service the following service operations are defined:
-	Update;
-	Subscribe;
-	Unsubscribe;
-	Notify;
-	Delete;
-	Options.
5.3.2.2	Update Service Operation
5.3.2.2.1	General
The Update operation shall be used by an NF Service Consumer (e.g. AMF) to update the NSSF with the S-NSSAIs the NF service consumer (e.g. AMF) supports per TA, and get the availability of the S-NSSAIs per TA for the S-NSSAIs the NF service consumer (e.g. AMF) supports.
The Update operation may also be used by an NF Service Consumer (e.g. AMF) to update the NSSF with the NSAG(s) associated with the S-NSSAI(s) supported by the NF Service Consumer (e.g. AMF) on per TA basis, and to get the availability of the NSAG(s) per TA for the NSAG(s) supported by the NF Service Consumer (e.g. AMF).
Figure 5.3.2.2.1-1: Update the S-NSSAIs the AMF supports per TA
1.	The NF service consumer (e.g. AMF) shall send a PUT request to the resource representing the NSSAI Availability information of the individual NF, identified by the {nfId}, to replace or create the NSSAI Availability information of the NF.
The content shall contain the NssaiAvailabilityInfo which contains one or more representations of the individual supportedSnssai information to be replaced.
The NssaiAvailabilityInfo in the content may contain NSAG information.
The NF service consumer (e.g. AMF) shall send a PATCH request to the resource representing the NSSAI Availability information of the individual NF, identified by the {nfId}, to update the NSSAI Availability information of the NF.
The content shall contain the PatchDocument which contains one or more PatchItem instructions for updating the individual supportedSnssai resources.
The content may contain the PatchDocument which contains one or more PatchItem instructions for updating the NSAG information.
2.	On success, "204 No content" shall be returned if Authorized NSSAI Availability is empty after the update; otherwise, "200 OK" shall be returned, the content of the PUT/PATCH response shall contain the representation describing the status of the request and the complete AuthorizedNssaiAvailabilityData information representing the current state of the AuthorizedNssaiAvailabilityInfo.
If there is no supported S-NSSAIs authorized by the NSSF for the TA, the NSSF shall not return the AuthorizedNssaiAvailabilityData for the corresponding TA in the response.
On failure or redirection, the NSSF shall return one of the HTTP status code together with the response body listed in Table 6.2.3.2.3.1-2 / Table 6.2.3.2.3.2-2.
5.3.2.3	Subscribe Service Operation
5.3.2.3.1	Creation of a subscription
The Subscribe Operation is used by a NF Service Consumer (e.g. AMF, V-NSSF) to subscribe to a notification of:
-	Network Slice Replacement;
-	Network Slice Instance Replacement;
-	any changes in status of the NSSAI availability information (e.g. S-NSSAIs available per TA and the restricted S-NSSAI(s) per PLMN in that TA in the serving PLMN of the UE) when updated by another AMF (as specified in clause 5.2.16.3.4 of 3GPP TS 23.502 [3]); and/or
-	any changes in the status of the NSSAI validity time information.
Figure 5.3.2.3.1-1 Create a subscription
1.	The NF Service Consumer shall send a POST request to create a subscription resource in the NSSF. The content of the POST request shall contain a representation of the individual event subscription resource to be created in the NssfEventSubscriptionCreateData.
The request shall indicate the type(s) of events for which the subscription is created, i.e., Network Slice Replacement, Network Slice Instance Replacement, any changes in the status of the NSSAI validity time information and/or of any changes in status of the NSSAI availability information.
The request may contain an expiry time, suggested by the NF Service Consumer as a hint, representing the time upto during which the subscription is desired to be kept active and describes the maximum duration after which the subscribed event shall stop generating report.
The request may also indicate a specific AMF Set to restrict the subscriptions to notifications applicable to the AMF Set (i.e. notifications related to S-NSSAIs supported by the AMF Set). If the AMF Set is provided and the NSSF support the "SATAS" feature (see clause 6.2.8), the NF Service Consumer may also indicate that the subscription is for all TAI(s) associated with the AMF Set.
If the service operation is invoked for subscription to Network Slice Replacement notification, then the request shall contain:
-	for VPLMN S-NSSAI: the list of S-NSSAIs in the VPLMN served by the NF Service Consumer that may be replaced with, the NF type of the NF Service Consumer (e.g., AMF) and the NF instance ID of the requester NF.
-	for HPLMN S-NSSAI: the list of S-NSSAIs in the HPLMN that the S-NSSAI may be replaced with, the NF type of the NF Service Consumer (e.g., AMF, V-NSSF), the NF instance ID of the requester NF and the HPLMN ID.
If the service operation is invoked for subscription to Network Slice Instance Replacement notification, then the request shall contain:
-	the list of S-NSSAIs and/or the list of NSI IDs that may become congested or no longer available.
2.	On success, "201 Created" shall be returned, and the content of the POST response shall contain the representation describing the status of the created subscription in NssfEventSubscriptionCreatedData.
For a subscription to any changes in status of the NSSAI availability information, the NssfEventSubscriptionCreatedData may contain the AuthorizedNssaiAvailabilityData information, if available.
If there is no supported S-NSSAIs authorized by the NSSF for the TA, the NSSF shall not return the AuthorizedNssaiAvailabilityData for the corresponding TA in the response.
The Location header shall contain the location (URI) of the created subscription resource.
The response, based on operator policy and taking into account the expiry time included in the request, may contain the expiry time, as determined by the NSSF, after which the subscription becomes invalid. Once the subscription expires, if the NF Service Consumer wants to keep receiving notifications, it shall create a new subscription in the NSSF. The NSSF shall not provide the same expiry time for many subscriptions in order to avoid all of them expiring and recreating the subscription at the same time. If the expiry time is not included in the response, the NF Service Consumer shall consider the subscription to be valid without an expiry time.
If the request is subscribing to more than one events, the response may contain the acceptedEvents IE to indicate the list of events that are accepted by the NSSF in the created subscription.
On failure or redirection, the NSSF shall return one of the HTTP status code together with the response body listed in Table 6.2.3.3.3.1-2.
5.3.2.3.2	Modification of a subscription
The Subscribe Operation may be used by a NF Service Consumer (e.g. AMF, V-NSSF) towards an NSSF supporting the SUMOD feature, when it needs to modify an existing subscription previously created by itself.
The NF Service Consumer shall modify the subscription by using HTTP method PATCH with the URI of the individual subscription resource to be modified.
Figure 5.3.2.3.2-1 Modify a subscription
1.	The NF Service Consumer (e.g. AMF, V-NSSF) shall send a PATCH request to the resource URI identifying the individual subscription resource. The content shall contain the PatchDocument which contains one or more PatchItem instructions for updating the subscription data.
The NF Service Consumer shall not change the event IE included in the NssfEventSubscriptionCreateData by invoking the PATCH request message.
For a subscription to any changes in status of the NSSAI availability information, the taiList IE may only be set to an empty array in PATCH request if the NF service consumer and NSSF support the ONSSAI feature.
2.	On success, "200 OK" shall be returned, the content of the PATCH response shall contain the representation describing the updated subscription in NssfEventSubscriptionCreatedData.
For a subscription to any changes in status of the NSSAI availability information, if there is no supported S-NSSAIs authorized by the NSSF for the TA, the NSSF shall not return the AuthorizedNssaiAvailabilityData for the corresponding TA in the response.
On failure or redirection, the NSSF shall return one of the HTTP status code together with the response body listed in Table 6.2.3.4.3.2-2.
5.3.2.4	Unsubscribe Service Operation
5.3.2.4.1	General
The Unsubscribe Operation is used by a NF Service Consumer (e.g. AMF, V-NSSF) to unsubscribe to a notification of any previously subscribed changes to the NSSAI availability information, Network Slice Replacement, Network Slice Instance Replacement and/or NSSAI validity time information.
Figure 5.3.2.4.1-1 Unsubscribe a subscription
1.	The NF Service Consumer shall send a DELETE request to delete an existing subscription resource in the NSSF.
2.	If the request is accepted, the NSSF shall respond with the status code 204 indicating the resource identified by subscription ID is successfully deleted.
On failure or redirection, the NSSF shall return one of the HTTP status code together with the response body listed in Table 6.2.3.4.3.1-2.
5.3.2.5	Notify Service Operation
5.3.2.5.1	General
The Notify Service operation shall be used by the NSSF to update the NF Service Consumer (e.g. AMF) with any change in status, on a per TA basis, of the S-NSSAIs available per TA (unrestricted) and the S-NSSAIs restricted per PLMN in that TA in the serving PLMN of the UE.
The service operation is also used to notify the NF Service Consumer (e.g., AMF, V-NSSF) of Network Slice Replacement and/or Network Slice Instance Replacement.
The service operation is also used to notify the NF Service Consumer (e.g., AMF) of any changes in the status of the NSSAI validity time information.
Figure 5.3.2.5.1-1: Update the AMF with any S-NSSAIs restricted per TA
1.	The NSSF shall send a POST request to the resource representing the NSSF availability resource in the NF service consumer (e.g. AMF, V-NSSF). The content of the POST request shall contain the one representations of the individual NssfEventNotification resource.
For a subscription to any changes in status of the NSSAI availability information:
-	If there is no supported S-NSSAIs authorized by the NSSF for the TA, the NSSF shall not return the AuthorizedNssaiAvailabilityData for the corresponding TA in the notification.
-	If there is no supported S-NSSAIs authorized by the NSSF for all TAs and the NF Service Consumer has indicated support of "EANAN" feature, the NSSF shall set authorizedNssaiAvailabilityData attribute to an empty array.
For a subscription to Network Slice Instance Replacement:
-	If the NSSF supports the NSIUN feature (see clause 6.2.8) and if the Network Slice instance becomes no longer available, the NSSF shall notify the NF Service Consumer (e.g., AMF, V-NSSF) having subscribed to this event for the related S-NSSAI and/or NSI ID that the NSI is no longer available. In that case, the POST request from the NSSF shall contain the list of S-NSSAIs and the associated NSI IDs for which the status is changed (e.g., which become congested or no longer available). The request may also contain congestion mitigation information.
For a subscription to Network Slice Replacement:
-	If the NSSF supports the NSRP feature (see clause 6.2.8) and if the NSSF detects that an S-NSSAI becomes unavailable (e.g., based on OAM or NWDAF analytics output), the NSSF shall send Network Slice Replacement for the S-NSSAI to the NF service consumer if the NF service consumer has subscribed to this event for the related S-NSSAI. The notification shall include an alternative S-NSSAI which can be used by the NF service consumer to replace the unavailable S-NSSAI. In case of roaming, the notification shall include:
-	for VPLMN S-NSSAI replacement: the alternative VPLMN S-NSSAI for the S-NSSAI and the corresponding mapping to the S-NSSAI to be replaced.
-	for HPLMN S-NSSAI replacement: the alternative HPLMN S-NSSAI for the S-NSSAI and the corresponding mapping to the HPLMN S-NSSAI to be replaced and the HPLMN ID.
-	The NSSF shall notify the NF service consumer when the S-NSSAI becomes available again. The notification shall contain:
-	an indication to stop Network Slice replacement for new UEs; or
-	an indication to terminate Network Slice Replacement for all the UEs and move back the PDU sessions from the alternative S-NSSAI to the S-NSSAI.
-	The NSSF may provide to the NF service consumer congestion mitigation information for Network Slice Replacement that may contain:
-	the percentage of registered UEs that is applied for Network Slice Replacement; or
-	an indication that the Network Slice Replacement applies to new UEs registering with the S-NSSAI.
If the notification is triggered by the AMF that updates the supported S-NSSAIs per TA by using the Update operation, the NSSF shall not send the notification to the same AMF.
For a subscription to any changes in the status of the NSSAI validity time information:
-	the NSSF shall notify the NF service consumer when the validity timer related to the subscribed S-NSSAI(s) are changed by including the S-NSSAI(s) and associated validity time for each S-NSSAI.
2.	On success, "204 No Content" shall be returned and the content of the POST response shall be empty.
On failure or redirection, the NF service consumer shall return one of the HTTP status code together with the response body listed in Table 6.2.5.2.3.1-2.
5.3.2.6	Delete Service Operation
5.3.2.6.1	General
The Delete Service operation shall be used by the NF service consumer (e.g. AMF) to delete the NSSAI availability information stored for the NF service consumer in the NSSF.
Figure 5.3.2.6.1-1: Delete the NSSAI Availability Information at NSSF
1.	The NF service consumer (e.g. AMF) shall send a DELETE request to remove the NSSAI availability information for the NF service consumer represented by the {nfId} (e.g. AMF ID).
2.	The NSSF shall delete the NSSAI Availability information for the individual AMF and shall return the 204 No Content status code.
On failure or redirection, the NSSF shall return one of the HTTP status code together with the response body listed in Table 6.2.3.2.3.3-2.
5.3.2.7	Options Service Operation
5.3.2.7.1	General
The Options service operation is used by a NF Service Consumer (e.g. AMF) to discover the communication options supported by the NSSF for the resource.
Figure 5.3.2.7.1-1: Procedure for the discovery of communication options supported by the NSSF
1.	The NF service consumer (e.g. AMF) shall send an OPTIONS request to discover the communication options supported by the NSSF for the resource.
2.	If the request is accepted, the NSSF shall respond with the status code 200 OK and include an Accept-Encoding header (as described in IETF RFC 9110 [18]).
On failure or redirection, the NSSF shall return one of the HTTP status code listed in Table 6.2.3.5.3.1-3.
