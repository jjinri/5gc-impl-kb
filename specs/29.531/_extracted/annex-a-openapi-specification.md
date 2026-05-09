---
source_spec: "29.531"
source_path: specs/29.531/29531-j60.docx
source_mtime: 1778311321.968539
section: "Annex A"
title: "Annex A (normative): OpenAPI specification"
generator: spec-split.py
generator_version: 1
chars: 45376
---

# Annex A (normative): OpenAPI specification

Annex A (normative):
OpenAPI specification
A.1	General
This Annex specifies the formal definition of the service provided by NSSF in this document. It consists of OpenAPI 3.0.0 specifications, in YAML format.
This Annex takes precedence when being discrepant to other parts of the specification with respect to the encoding of information elements and methods within the API(s).
NOTE:	The semantics and procedures, as well as conditions, e.g. for the applicability and allowed combinations of attributes or values, not expressed in the OpenAPI definitions but defined in other parts of the specification also apply.
Informative copies of the OpenAPI specification files contained in this 3GPP Technical Specification are available on a Git-based repository that uses the GitLab software version control system (see 3GPP TS 29.501 [5] clause 5.3.1 and 3GPP TR 21.900 [7] clause 5B).
A.2	Nnssf_NSSelection API
openapi: 3.0.0
info:
  version: '2.4.0'
  title: 'NSSF NS Selection'
  description: |
    NSSF Network Slice Selection Service.  
    © 2025, 3GPP Organizational Partners (ARIB, ATIS, CCSA, ETSI, TSDSI, TTA, TTC).  
    All rights reserved.
security:
  - {}
  - oAuth2ClientCredentials:
    - nnssf-nsselection
servers:
  - url: '{apiRoot}/nnssf-nsselection/v2'
    variables:
      apiRoot:
        default: https://example.com
        description: apiRoot as defined in clause 4.4 of 3GPP TS 29.501
externalDocs:
  description: 3GPP TS 29.531 V19.5.0; 5G System; Network Slice Selection Services; Stage 3
  url: https://www.3gpp.org/ftp/Specs/archive/29_series/29.531/
paths:
  /network-slice-information:
    get:
      summary:  Retrieve the Network Slice Selection Information
      tags:
        - Network Slice Information (Document)
      operationId: NSSelectionGet
      parameters:
        - name: nf-type
          in: query
          description: NF type of the NF service consumer
          required: true
          schema:
            $ref: 'TS29510_Nnrf_NFManagement.yaml#/components/schemas/NFType'
        - name: nf-id
          in: query
          description: NF Instance ID of the NF service consumer
          required: true
          schema:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/NfInstanceId'
        - name: slice-info-request-for-registration
          in: query
          description: Requested network slice information during Registration procedure
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/SliceInfoForRegistration'
        - name: slice-info-request-for-pdu-session
          in: query
          description: >
            Requested network slice information during PDU session establishment procedure
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/SliceInfoForPDUSession'
        - name: slice-info-request-for-ue-cu
          in: query
          description: Requested network slice information during UE confuguration update procedure
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/SliceInfoForUEConfigurationUpdate'
        - name: slice-info-request-for-pdn-connection
          in: query
          description: >
            Requested network slice information during PDN Connection establishment procedure
          content:
            application/json:
              schema:
                type: array
                items:
                  $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
                minItems: 1
        - name: slice-info-request-for-other-purpose
          in: query
          description: >
            Requested network slice information, e.g. during Network Slice load analytics
          content:
            application/json:
              schema:
                type: array
                items:
                  $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
                minItems: 1
        - name: home-plmn-id
          in: query
          description: PLMN ID of the HPLMN
          content:
            application/json:
              schema:
                $ref: 'TS29571_CommonData.yaml#/components/schemas/PlmnId'
        - name: tai
          in: query
          description: TAI of the UE
          content:
            application/json:
              schema:
                $ref: 'TS29571_CommonData.yaml#/components/schemas/Tai'
        - name: supported-features
          in: query
          description: Features required to be supported by the NFs in the target slice instance
          schema:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/SupportedFeatures'
      responses:
        '200':
          description: OK (Successful Network Slice Selection)
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/AuthorizedNetworkSliceInfo'
        '307':
          $ref: 'TS29571_CommonData.yaml#/components/responses/307'
        '308':
          $ref: 'TS29571_CommonData.yaml#/components/responses/308'
        '400':
          $ref: 'TS29571_CommonData.yaml#/components/responses/400'
        '401':
          $ref: 'TS29571_CommonData.yaml#/components/responses/401'
        '403':
          $ref: 'TS29571_CommonData.yaml#/components/responses/403'
        '404':
          $ref: 'TS29571_CommonData.yaml#/components/responses/404'
        '406':
          $ref: 'TS29571_CommonData.yaml#/components/responses/406'
        '414':
          $ref: 'TS29571_CommonData.yaml#/components/responses/414'
        '429':
          $ref: 'TS29571_CommonData.yaml#/components/responses/429'
        '500':
          $ref: 'TS29571_CommonData.yaml#/components/responses/500'
        '502':
          $ref: 'TS29571_CommonData.yaml#/components/responses/502'
        '503':
          $ref: 'TS29571_CommonData.yaml#/components/responses/503'
        default:
          description: Unexpected error
components:
  securitySchemes:
    oAuth2ClientCredentials:
      type: oauth2
      flows:
        clientCredentials:
          tokenUrl: '{nrfApiRoot}/oauth2/token'
          scopes:
            nnssf-nsselection: Access to the Nnssf_NSSelection API
  schemas:
#
# STRUCTURED TYPES
#
    AuthorizedNetworkSliceInfo:
      description: Contains the authorized network slice information
      type: object
      properties:
        allowedNssaiList:
          type: array
          items:
            $ref: '#/components/schemas/AllowedNssai'
          minItems: 1
        configuredNssai:
          type: array
          items:
            $ref: '#/components/schemas/ConfiguredSnssai'
          minItems: 1
        targetAmfSet:
          type: string
          pattern: '^[0-9]{3}-[0-9]{2,3}-[A-Fa-f0-9]{2}-[0-3][A-Fa-f0-9]{2}$'
        candidateAmfList:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/NfInstanceId'
          minItems: 1
        rejectedNssaiInPlmn:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
          minItems: 1
        rejectedNssaiInTa:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
          minItems: 1
        nsiInformation:
          $ref: '#/components/schemas/NsiInformation'
        supportedFeatures:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/SupportedFeatures'
        nrfAmfSet:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Uri'
        nrfAmfSetNfMgtUri:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Uri'
        nrfAmfSetAccessTokenUri:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Uri'
        nrfOauth2Required:
          type: object
          description: >
            Map indicating whether the NRF requires Oauth2-based authorization for accessing
            its services. The key of the map shall be the name of an NRF service,
            e.g. "nnrf-nfm" or "nnrf-disc"
          additionalProperties:
            type: boolean
          minProperties: 1
        targetAmfServiceSet:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/NfServiceSetId'
        targetNssai:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
          minItems: 1
        nsagInfos:
          type: array
          items:
            $ref: '#/components/schemas/NsagInfo'
          minItems: 1
        mappingOfNssai:
          type: array
          items:
            $ref: '#/components/schemas/MappingOfSnssai'
          minItems: 1
        snssaiInfoRspData:
          description: >
            A map (list of key-value pairs) where Snssai serves as key of SnssaiInfo
          type: object
          additionalProperties:
            $ref: '#/components/schemas/SnssaiInfo'
          minProperties: 1
    SubscribedSnssai:
      description: Contains the subscribed S-NSSAI
      type: object
      required:
        - subscribedSnssai
      properties:
        subscribedSnssai:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
        defaultIndication:
          type: boolean
        subscribedNsSrgList:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/NsSrg'
          minItems: 1
    AllowedSnssai:
      description: >
        Contains the authorized S-NSSAI and optional mapped home S-NSSAI and
        network slice instance information
      type: object
      required:
        - allowedSnssai
      properties:
        allowedSnssai:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
        nsiInformationList:
          type: array
          items:
            $ref: '#/components/schemas/NsiInformation'
          minItems: 1
        mappedHomeSnssai:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
    AllowedNssai:
      description: >
        Contains an array of allowed S-NSSAI that constitute the allowed NSSAI information
        for the authorized network slice information
      type: object
      required:
        - allowedSnssaiList
        - accessType
      properties:
        allowedSnssaiList:
          type: array
          items:
            $ref: '#/components/schemas/AllowedSnssai'
          minItems: 1
        accessType:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/AccessType'
    NsiInformation:
      description: >
        Contains the API URIs of NRF services to be used to discover NFs/services,
        subscribe to NF status changes and/or request access tokens within the selected
        Network Slice instance and optional the Identifier of the selected Network Slice instance
      type: object
      required:
        - nrfId
      properties:
        nrfId:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Uri'
        nsiId:
          $ref: '#/components/schemas/NsiId'
        nrfNfMgtUri:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Uri'
        nrfAccessTokenUri:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Uri'
        nrfOauth2Required:
          type: object
          description: >
            Map indicating whether the NRF requires Oauth2-based authorization for accessing
            its services. The key of the map shall be the name of an NRF service,
            e.g. "nnrf-nfm" or "nnrf-disc"
          additionalProperties:
            type: boolean
          minProperties: 1
    MappingOfSnssai:
      description: >
        Contains the mapping of S-NSSAI in the serving network and the value of the home network
      type: object
      required:
        - servingSnssai
        - homeSnssai
      properties:
        servingSnssai:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
        homeSnssai:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
    SliceInfoForRegistration:
      description: Contains the slice information requested during a Registration procedure
      type: object
      properties:
        subscribedNssai:
          type: array
          items:
            $ref: '#/components/schemas/SubscribedSnssai'
          minItems: 1
        allowedNssaiCurrentAccess:
          $ref: '#/components/schemas/AllowedNssai'
        allowedNssaiOtherAccess:
          $ref: '#/components/schemas/AllowedNssai'
        sNssaiForMapping:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
          minItems: 1
        requestedNssai:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
          minItems: 1
        defaultConfiguredSnssaiInd:
          type: boolean
          default: false
        mappingOfNssai:
          type: array
          items:
            $ref: '#/components/schemas/MappingOfSnssai'
          minItems: 1
        requestMapping:
          type: boolean
        ueSupNssrgInd:
          type: boolean
        suppressNssrgInd:
          type: boolean
        nsagSupported:
          type: boolean
          default: false
    SliceInfoForPDUSession:
      description: >
        Contains the slice information requested during PDU Session establishment procedure
      type: object
      required:
        - sNssai
        - roamingIndication
      properties:
        sNssai:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
        roamingIndication:
          $ref: '#/components/schemas/RoamingIndication'
        homeSnssai:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
    SliceInfoForUEConfigurationUpdate:
      description: >
        Contains the slice information requested during UE configuration update procedure
      type: object
      properties:
        subscribedNssai:
          type: array
          items:
            $ref: '#/components/schemas/SubscribedSnssai'
          minItems: 1
        allowedNssaiCurrentAccess:
          $ref: '#/components/schemas/AllowedNssai'
        allowedNssaiOtherAccess:
          $ref: '#/components/schemas/AllowedNssai'
        defaultConfiguredSnssaiInd:
          type: boolean
          default: false
        requestedNssai:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
          minItems: 1
        mappingOfNssai:
          type: array
          items:
            $ref: '#/components/schemas/MappingOfSnssai'
          minItems: 1
        ueSupNssrgInd:
          type: boolean
        suppressNssrgInd:
          type: boolean
        rejectedNssaiRa:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
          minItems: 1
        nsagSupported:
          type: boolean
          default: false
    ConfiguredSnssai:
      description: >
        Contains the configured S-NSSAI authorized by the NSSF in the serving PLMN and optional mapped home S-NSSAI
      type: object
      required:
        - configuredSnssai
      properties:
        configuredSnssai:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
        mappedHomeSnssai:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
    NsagInfo:
      description: >
        Contains the association of NSAGs and S-NSSAI(s) along with the TA(s) within which
        the association is valid.
      type: object
      required:
        - nsagIds
        - snssaiList
      properties:
        nsagIds:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/NsagId'
          minItems: 1
        snssaiList:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
          minItems: 1
        taiList:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Tai'
          minItems: 1
        taiRangeList:
          type: array
          items:
            $ref: 'TS29510_Nnrf_NFManagement.yaml#/components/schemas/TaiRange'
          minItems: 1
    SnssaiInfo:
      description: Contains the slice information in the response from NSSF
      type: object
      properties:
        nsiIds:
          type: array
          items:
            $ref: '#/components/schemas/NsiId'
          minItems: 1
#
# SIMPLE TYPES
#
    NsiId:
      description: Contains the Identifier of the selected Network Slice instance
      type: string
#
# ENUMS
#
    RoamingIndication:
      description: Contains the indication on roaming
      anyOf:
        - type: string
          enum:
            - NON_ROAMING
            - LOCAL_BREAKOUT
            - HOME_ROUTED_ROAMING
        - type: string
A.3	Nnssf_NSSAIAvailability API
openapi: 3.0.0
info:
  version: '1.4.0'
  title: 'NSSF NSSAI Availability'
  description: |
    NSSF NSSAI Availability Service.  
    © 2025, 3GPP Organizational Partners (ARIB, ATIS, CCSA, ETSI, TSDSI, TTA, TTC).  
    All rights reserved.
security:
  - {}
  - oAuth2ClientCredentials:
    - nnssf-nssaiavailability
servers:
  - url: '{apiRoot}/nnssf-nssaiavailability/v1'
    variables:
      apiRoot:
        default: https://example.com
        description: apiRoot as defined in clause 4.4 of 3GPP TS 29.501
externalDocs:
  description: 3GPP TS 29.531 V19.5.0; 5G System; Network Slice Selection Services; Stage 3
  url: https://www.3gpp.org/ftp/Specs/archive/29_series/29.531/
paths:
  /nssai-availability/{nfId}:
    put:
      summary: Updates/replaces the NSSF with the S-NSSAIs the NF service consumer (e.g AMF)supports per TA
      tags:
        - NF Instance ID (Document)
      operationId: NSSAIAvailabilityPut
      parameters:
        - name: nfId
          in: path
          description: Identifier of the NF service consumer instance
          required: true
          schema:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/NfInstanceId'
        - name: Content-Encoding
          in: header
          description: Content-Encoding, described in IETF RFC 9110
          schema:
            type: string
        - name: Accept-Encoding
          in: header
          description: Accept-Encoding, described in IETF RFC 9110
          schema:
            type: string
      requestBody:
        description: Parameters to update/replace at the NSSF, the S-NSSAIs supported per TA
        required: true
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/NssaiAvailabilityInfo'
      responses:
        '200':
          description: OK (Successful update of SNSSAI information per TA)
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/AuthorizedNssaiAvailabilityInfo'
          headers:
            Accept-Encoding:
              description: Accept-Encoding, described in IETF RFC 9110
              schema:
                type: string
            Content-Encoding:
              description: Content-Encoding, described in IETF RFC 9110
              schema:
                type: string
        '204':
          description: No Content (No supported slices after Successful update)
        '307':
          $ref: 'TS29571_CommonData.yaml#/components/responses/307'
        '308':
          $ref: 'TS29571_CommonData.yaml#/components/responses/308'
        '400':
          $ref: 'TS29571_CommonData.yaml#/components/responses/400'
        '401':
          $ref: 'TS29571_CommonData.yaml#/components/responses/401'
        '403':
          $ref: 'TS29571_CommonData.yaml#/components/responses/403'
        '404':
          $ref: 'TS29571_CommonData.yaml#/components/responses/404'
        '411':
          $ref: 'TS29571_CommonData.yaml#/components/responses/411'
        '413':
          $ref: 'TS29571_CommonData.yaml#/components/responses/413'
        '415':
          $ref: 'TS29571_CommonData.yaml#/components/responses/415'
        '429':
          $ref: 'TS29571_CommonData.yaml#/components/responses/429'
        '500':
          $ref: 'TS29571_CommonData.yaml#/components/responses/500'
        '502':
          $ref: 'TS29571_CommonData.yaml#/components/responses/502'
        '503':
          $ref: 'TS29571_CommonData.yaml#/components/responses/503'
        default:
          description: Unexpected error
    patch:
      summary: Updates an already existing S-NSSAIs per TA provided by the NF service consumer (e.g AMF)
      tags:
        - NF Instance ID (Document)
      operationId: NSSAIAvailabilityPatch
      parameters:
        - name: nfId
          in: path
          description: Identifier of the NF service consumer instance
          required: true
          schema:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/NfInstanceId'
        - name: Content-Encoding
          in: header
          description: Content-Encoding, described in IETF RFC 9110
          schema:
            type: string
        - name: Accept-Encoding
          in: header
          description: Accept-Encoding, described in IETF RFC 9110
          schema:
            type: string
      requestBody:
        description: JSON Patch instructions to update at the NSSF, the S-NSSAIs supported per TA
        required: true
        content:
          application/json-patch+json::
            schema:
              $ref: '#/components/schemas/PatchDocument'
      responses:
        '200':
          description: OK (Successful update of SNSSAI information per TA)
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/AuthorizedNssaiAvailabilityInfo'
          headers:
            Accept-Encoding:
              description: Accept-Encoding, described in IETF RFC 9110
              schema:
                type: string
            Content-Encoding:
              description: Content-Encoding, described in IETF RFC 9110
              schema:
                type: string
        '204':
          description: No Content (No supported slices after Successful update)
        '307':
          $ref: 'TS29571_CommonData.yaml#/components/responses/307'
        '308':
          $ref: 'TS29571_CommonData.yaml#/components/responses/308'
        '400':
          $ref: 'TS29571_CommonData.yaml#/components/responses/400'
        '401':
          $ref: 'TS29571_CommonData.yaml#/components/responses/401'
        '403':
          $ref: 'TS29571_CommonData.yaml#/components/responses/403'
        '404':
          $ref: 'TS29571_CommonData.yaml#/components/responses/404'
        '411':
          $ref: 'TS29571_CommonData.yaml#/components/responses/411'
        '413':
          $ref: 'TS29571_CommonData.yaml#/components/responses/413'
        '415':
          $ref: 'TS29571_CommonData.yaml#/components/responses/415'
        '429':
          $ref: 'TS29571_CommonData.yaml#/components/responses/429'
        '500':
          $ref: 'TS29571_CommonData.yaml#/components/responses/500'
        '502':
          $ref: 'TS29571_CommonData.yaml#/components/responses/502'
        '503':
          $ref: 'TS29571_CommonData.yaml#/components/responses/503'
        default:
          description: Unexpected error
    delete:
      summary: Deletes an already existing S-NSSAIs per TA provided by the NF service consumer (e.g AMF)
      tags:
        - NF Instance ID (Document)
      operationId: NSSAIAvailabilityDelete
      parameters:
        - name: nfId
          in: path
          description: Identifier of the NF service consumer instance
          required: true
          schema:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/NfInstanceId'
      responses:
        '204':
          description: No Content (Successful deletion of SNSSAI information per TA)
        '307':
          $ref: 'TS29571_CommonData.yaml#/components/responses/307'
        '308':
          $ref: 'TS29571_CommonData.yaml#/components/responses/308'
        '400':
          $ref: 'TS29571_CommonData.yaml#/components/responses/400'
        '401':
          $ref: 'TS29571_CommonData.yaml#/components/responses/401'
        '403':
          $ref: 'TS29571_CommonData.yaml#/components/responses/403'
        '404':
          $ref: 'TS29571_CommonData.yaml#/components/responses/404'
        '429':
          $ref: 'TS29571_CommonData.yaml#/components/responses/429'
        '500':
          $ref: 'TS29571_CommonData.yaml#/components/responses/500'
        '502':
          $ref: 'TS29571_CommonData.yaml#/components/responses/502'
        '503':
          $ref: 'TS29571_CommonData.yaml#/components/responses/503'
        default:
          description: Unexpected error
  /nssai-availability/subscriptions:
    post:
      summary: Creates subscriptions for notification about updates to NSSAI availability information
      tags:
        - Subscriptions (Collection)
      operationId: NSSAIAvailabilityPost
      parameters:
        - name: Content-Encoding
          in: header
          description: Content-Encoding, described in IETF RFC 9110
          schema:
            type: string
      requestBody:
        description: Subscription for notification about updates to NSSAI availability information
        required: true
        content:
          application/json:
            schema:
              $ref: '#/components/schemas/NssfEventSubscriptionCreateData'
      callbacks:
        nssaiAvailabilityNotification:
          '{$request.body#/nfNssaiAvailabilityUri}':
            post:
              parameters:
                - name: Content-Encoding
                  in: header
                  description: Content-Encoding, described in IETF RFC 9110
                  schema:
                    type: string
              requestBody:  # contents of the callback message
                required: true
                content:
                  application/json:
                    schema:
                      $ref: '#/components/schemas/NssfEventNotification'
              responses:
                '204':
                  description: No Content (successful notification)
                  headers:
                    Accept-Encoding:
                      description: Accept-Encoding, described in IETF RFC 9110
                      schema:
                        type: string
                '307':
                  $ref: 'TS29571_CommonData.yaml#/components/responses/307'
                '308':
                  $ref: 'TS29571_CommonData.yaml#/components/responses/308'
                '400':
                  $ref: 'TS29571_CommonData.yaml#/components/responses/400'
                '401':
                  $ref: 'TS29571_CommonData.yaml#/components/responses/401'
                '403':
                  $ref: 'TS29571_CommonData.yaml#/components/responses/403'
                '404':
                  $ref: 'TS29571_CommonData.yaml#/components/responses/404'
                '411':
                  $ref: 'TS29571_CommonData.yaml#/components/responses/411'
                '413':
                  $ref: 'TS29571_CommonData.yaml#/components/responses/413'
                '415':
                  $ref: 'TS29571_CommonData.yaml#/components/responses/415'
                '429':
                  $ref: 'TS29571_CommonData.yaml#/components/responses/429'
                '500':
                  $ref: 'TS29571_CommonData.yaml#/components/responses/500'
                '502':
                  $ref: 'TS29571_CommonData.yaml#/components/responses/502'
                '503':
                  $ref: 'TS29571_CommonData.yaml#/components/responses/503'
                default:
                  description: Unexpected error
      responses:
        '201':
          description: Created (Successful creation of subscription for notification)
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/NssfEventSubscriptionCreatedData'
          headers:
            Location:
              description: >
                Contains the URI of the newly created resource, according to the structure:
                {apiRoot}/nnssf-nssaiavailability/<apiVersion>/nssai-availability/subscriptions/{subscriptionId}
              required: true
              schema:
                type: string
            Content-Encoding:
              description: Content-Encoding, described in IETF RFC 9110
              schema:
                type: string
        '307':
          $ref: 'TS29571_CommonData.yaml#/components/responses/307'
        '308':
          $ref: 'TS29571_CommonData.yaml#/components/responses/308'
        '400':
          $ref: 'TS29571_CommonData.yaml#/components/responses/400'
        '401':
          $ref: 'TS29571_CommonData.yaml#/components/responses/401'
        '403':
          $ref: 'TS29571_CommonData.yaml#/components/responses/403'
        '404':
          $ref: 'TS29571_CommonData.yaml#/components/responses/404'
        '411':
          $ref: 'TS29571_CommonData.yaml#/components/responses/411'
        '413':
          $ref: 'TS29571_CommonData.yaml#/components/responses/413'
        '415':
          $ref: 'TS29571_CommonData.yaml#/components/responses/415'
        '429':
          $ref: 'TS29571_CommonData.yaml#/components/responses/429'
        '500':
          $ref: 'TS29571_CommonData.yaml#/components/responses/500'
        '501':
          $ref: 'TS29571_CommonData.yaml#/components/responses/501'
        '502':
          $ref: 'TS29571_CommonData.yaml#/components/responses/502'
        '503':
          $ref: 'TS29571_CommonData.yaml#/components/responses/503'
        default:
          description: Unexpected error
  /nssai-availability/subscriptions/{subscriptionId}:
    delete:
      summary: Deletes an already existing NSSAI availability notification subscription
      tags:
        - Subscription ID (Document)
      operationId: NSSAIAvailabilityUnsubscribe
      parameters:
        - name: subscriptionId
          in: path
          description: Identifier of the subscription for notification
          required: true
          schema:
            type: string
      responses:
        '204':
          description: >
            No Content (Successful deletion of subscription for NSSAI Availability notification)
        '307':
          $ref: 'TS29571_CommonData.yaml#/components/responses/307'
        '308':
          $ref: 'TS29571_CommonData.yaml#/components/responses/308'
        '400':
          $ref: 'TS29571_CommonData.yaml#/components/responses/400'
        '401':
          $ref: 'TS29571_CommonData.yaml#/components/responses/401'
        '403':
          $ref: 'TS29571_CommonData.yaml#/components/responses/403'
        '404':
          $ref: 'TS29571_CommonData.yaml#/components/responses/404'
        '429':
          $ref: 'TS29571_CommonData.yaml#/components/responses/429'
        '500':
          $ref: 'TS29571_CommonData.yaml#/components/responses/500'
        '502':
          $ref: 'TS29571_CommonData.yaml#/components/responses/502'
        '503':
          $ref: 'TS29571_CommonData.yaml#/components/responses/503'
        default:
          description: Unexpected error
    patch:
      summary: updates an already existing NSSAI availability notification subscription
      tags:
        - Subscription ID (Document)
      operationId: NSSAIAvailabilitySubModifyPatch
      parameters:
        - name: subscriptionId
          in: path
          description: Identifier of the subscription for notification
          required: true
          schema:
            type: string
        - name: Content-Encoding
          in: header
          description: Content-Encoding, described in IETF RFC 9110
          schema:
            type: string
      requestBody:
        description: >
          JSON Patch instructions to update at the NSSF,
          the NSSAI availability notification subscription
        required: true
        content:
          application/json-patch+json::
            schema:
              $ref: '#/components/schemas/PatchDocument'
      responses:
        '200':
          description: OK (Successful update of NSSAI availability notification subscription)
          content:
            application/json:
              schema:
                $ref: '#/components/schemas/NssfEventSubscriptionCreatedData'
          headers:
            Content-Encoding:
              description: Content-Encoding, described in IETF RFC 9110
              schema:
                type: string
        '307':
          $ref: 'TS29571_CommonData.yaml#/components/responses/307'
        '308':
          $ref: 'TS29571_CommonData.yaml#/components/responses/308'
        '400':
          $ref: 'TS29571_CommonData.yaml#/components/responses/400'
        '401':
          $ref: 'TS29571_CommonData.yaml#/components/responses/401'
        '403':
          $ref: 'TS29571_CommonData.yaml#/components/responses/403'
        '404':
          $ref: 'TS29571_CommonData.yaml#/components/responses/404'
        '411':
          $ref: 'TS29571_CommonData.yaml#/components/responses/411'
        '413':
          $ref: 'TS29571_CommonData.yaml#/components/responses/413'
        '415':
          $ref: 'TS29571_CommonData.yaml#/components/responses/415'
        '429':
          $ref: 'TS29571_CommonData.yaml#/components/responses/429'
        '500':
          $ref: 'TS29571_CommonData.yaml#/components/responses/500'
        '502':
          $ref: 'TS29571_CommonData.yaml#/components/responses/502'
        '503':
          $ref: 'TS29571_CommonData.yaml#/components/responses/503'
        default:
          description: Unexpected error
  /nssai-availability:
    options:
      summary: Discover communication options supported by NSSF for NSSAI Availability
      operationId: NSSAIAvailabilityOptions
      tags:
        - NSSAI Availability Store
      responses:
        '200':
          description: OK
          headers:
            Accept-Encoding:
              description: Accept-Encoding, described in IETF RFC 9110
              schema:
                type: string
        '307':
          $ref: 'TS29571_CommonData.yaml#/components/responses/307'
        '308':
          $ref: 'TS29571_CommonData.yaml#/components/responses/308'
        '400':
          $ref: 'TS29571_CommonData.yaml#/components/responses/400'
        '401':
          $ref: 'TS29571_CommonData.yaml#/components/responses/401'
        '403':
          $ref: 'TS29571_CommonData.yaml#/components/responses/403'
        '404':
          $ref: 'TS29571_CommonData.yaml#/components/responses/404'
        '405':
          $ref: 'TS29571_CommonData.yaml#/components/responses/405'
        '429':
          $ref: 'TS29571_CommonData.yaml#/components/responses/429'
        '500':
          $ref: 'TS29571_CommonData.yaml#/components/responses/500'
        '501':
          $ref: 'TS29571_CommonData.yaml#/components/responses/501'
        '502':
          $ref: 'TS29571_CommonData.yaml#/components/responses/502'
        '503':
          $ref: 'TS29571_CommonData.yaml#/components/responses/503'
        default:
          $ref: 'TS29571_CommonData.yaml#/components/responses/default'
components:
  securitySchemes:
    oAuth2ClientCredentials:
      type: oauth2
      flows:
        clientCredentials:
          tokenUrl: '{nrfApiRoot}/oauth2/token'
          scopes:
            nnssf-nssaiavailability: Access to the Nnssf_NSSAIAvailability API
  schemas:
#
# STRUCTURED TYPES
#
    NssaiAvailabilityInfo:
      description: This contains the Nssai availability information requested by the AMF
      type: object
      required:
        - supportedNssaiAvailabilityData
      properties:
        supportedNssaiAvailabilityData:
          type: array
          items:
            $ref: '#/components/schemas/SupportedNssaiAvailabilityData'
          minItems: 1
        supportedFeatures:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/SupportedFeatures'
        amfSetId:
          type: string
          pattern: '^[0-9]{3}-[0-9]{2,3}-[A-Fa-f0-9]{2}-[0-3][A-Fa-f0-9]{2}$'
    SupportedNssaiAvailabilityData:
      description: This contains the Nssai availability data information per TA supported by the AMF
      type: object
      required:
        - tai
        - supportedSnssaiList
      properties:
        tai:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Tai'
        supportedSnssaiList:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/ExtSnssai'
          minItems: 1
        taiList:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Tai'
          minItems: 1
        taiRangeList:
          type: array
          items:
            $ref: 'TS29510_Nnrf_NFManagement.yaml#/components/schemas/TaiRange'
          minItems: 1
        nsagInfos:
          type: array
          items:
            $ref: 'TS29531_Nnssf_NSSelection.yaml#/components/schemas/NsagInfo'
          minItems: 1
    AuthorizedNssaiAvailabilityData:
      description: This contains the Nssai availability data information per TA authorized by the NSSF
      type: object
      required:
        - tai
        - supportedSnssaiList
      properties:
        tai:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Tai'
        supportedSnssaiList:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/ExtSnssai'
          minItems: 1
        restrictedSnssaiList:
          type: array
          items:
            $ref: '#/components/schemas/RestrictedSnssai'
          minItems: 1
        taiList:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Tai'
          minItems: 1
        taiRangeList:
          type: array
          items:
            $ref: 'TS29510_Nnrf_NFManagement.yaml#/components/schemas/TaiRange'
          minItems: 1
        nsagInfos:
          type: array
          items:
            $ref: 'TS29531_Nnssf_NSSelection.yaml#/components/schemas/NsagInfo'
          minItems: 1
    RestrictedSnssai:
      description: This contains the restricted SNssai information per PLMN
      type: object
      required:
        - homePlmnId
        - sNssaiList
      properties:
        homePlmnId:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/PlmnId'
        sNssaiList:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/ExtSnssai'
          minItems: 1
        homePlmnIdList:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/PlmnId'
          minItems: 1
        roamingRestriction:
          type: boolean
          default: false
    AuthorizedNssaiAvailabilityInfo:
      description: This contains the Nssai availability data information authorized by the NSSF
      type: object
      required:
        - authorizedNssaiAvailabilityData
      properties:
        authorizedNssaiAvailabilityData:
          type: array
          items:
            $ref: '#/components/schemas/AuthorizedNssaiAvailabilityData'
          minItems: 1
        supportedFeatures:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/SupportedFeatures'
    NssfEventSubscriptionCreateData:
      description: This contains the information for event subscription
      type: object
      required:
        - nfNssaiAvailabilityUri
        - event
      properties:
        nfNssaiAvailabilityUri:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/Uri'
        taiList:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Tai'
        event:
          $ref: '#/components/schemas/NssfEventType'
        additionalEvents:
          type: array
          items:
            $ref: '#/components/schemas/NssfEventType'
          minItems: 1
        expiry:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/DateTime'
        amfSetId:
          type: string
          pattern: '^[0-9]{3}-[0-9]{2,3}-[A-Fa-f0-9]{2}-[0-3][A-Fa-f0-9]{2}$'
        taiRangeList:
          type: array
          items:
            $ref: 'TS29510_Nnrf_NFManagement.yaml#/components/schemas/TaiRange'
          minItems: 1
        amfId:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/NfInstanceId'
        supportedFeatures:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/SupportedFeatures'
        allAmfSetTaiInd:
          type: boolean
          default: false
        nsrpSubscribeInfo:
          $ref: '#/components/schemas/SnssaiReplacementSubscribeInfo'
        nsiunSubscribeInfo:
          $ref: '#/components/schemas/NsiUnavailabilitySubscribeInfo'
        validityTimeSubList:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
          minItems: 1
    NssfEventSubscriptionCreatedData:
      description: This contains the information for created event subscription
      type: object
      required:
        - subscriptionId
      properties:
        subscriptionId:
          type: string
        expiry:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/DateTime'
        authorizedNssaiAvailabilityData:
          type: array
          items:
            $ref: '#/components/schemas/AuthorizedNssaiAvailabilityData'
          minItems: 1
        supportedFeatures:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/SupportedFeatures'
        acceptedEvents:
          type: array
          items:
            $ref: '#/components/schemas/NssfEventType'
          minItems: 1
    NssfEventNotification:
      description: This contains the notification for created event subscription
      type: object
      required:
        - subscriptionId
      properties:
        subscriptionId:
          type: string
        authorizedNssaiAvailabilityData:
          type: array
          items:
            $ref: '#/components/schemas/AuthorizedNssaiAvailabilityData'
        altNssai:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/SnssaiReplaceInfo'
          minItems: 1
          description: >
            Indicate the impacted S-NSSAIs, the current status for each reported S-NSSAI, and 
            if available the alternative S-NSSAI per impacted S-NSSAI for the S-NSSAIs that are 
            reported as being not available.
        unavailableNsiList:
          type: array
          items:
            $ref: 'TS29531_Nnssf_NSSelection.yaml#/components/schemas/NsiId'
          minItems: 1
        nssaiValidityTimeInfo:
          deprecated: true
          description: >
            A map(list of key-value pairs where single Nssai serves as key)
            of the current validity time
          type: object
          additionalProperties:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/DateTime'
        nssaiValidityTimeInfoList:
          description: >
            A map(list of key-value pairs where single Nssai serves as key)
            of the current validity time information
          type: object
          additionalProperties:
            type: array
            items:
              $ref: 'TS29503_Nudm_SDM.yaml#/components/schemas/RecurTime'
            minItems: 1
          minProperties: 1
    SnssaiReplacementSubscribeInfo:
      description: >
        Present if the NF service consumer subscribes to events related to
        Network Slice Replacement.
      type: object
      properties:
        snssaiToSubscribe:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
        nfType:
          $ref: 'TS29510_Nnrf_NFManagement.yaml#/components/schemas/NFType'
        nfId:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/NfInstanceId'
        plmnId:
          $ref: 'TS29571_CommonData.yaml#/components/schemas/PlmnId'
      required:
        - snssaiToSubscribe
        - nfType
        - nfId
 
    NsiUnavailabilitySubscribeInfo:
      description: >
        Present if the NF service consumer subscribes to events related to
        Network Slice Instance Replacement.
      type: object
      properties:
        nsiToSubscribe:
          type: array
          items:
            $ref: 'TS29531_Nnssf_NSSelection.yaml#/components/schemas/NsiId'
        snssaiToSubscribe:
          type: array
          items:
            $ref: 'TS29571_CommonData.yaml#/components/schemas/Snssai'
    PatchDocument:
      description: >
        This contains the JSON Patch instructions for updating the Nssai availability data
        information at the NSSF
      type: array
      items:
        $ref: 'TS29571_CommonData.yaml#/components/schemas/PatchItem'
      minItems: 1
#
# SIMPLE TYPES
#
#
# ENUMS
#
    NssfEventType:
      description: This contains the event for the subscription
      anyOf:
        - type: string
          enum:
            - SNSSAI_STATUS_CHANGE_REPORT
            - SNSSAI_REPLACEMENT_REPORT
            - NSI_UNAVAILABILITY_REPORT
            - SNSSAI_VALIDITY_TIME_REPORT
        - type: string
