<?php

/**
 * @file
 * Stores handler functionality for OAI Direct Link responses.
 */

/**
 * Wrapper around islandora_oai_object_response_xml().
 *
 * @param array $params
 *   @see islandora_oai_object_response_xml().
 *
 * @return null|string
 *   XML string output of the record depending on configuration, NULL if an
 *   error was encountered.
 */
function islandora_oai_download_link_object_response_xml($params) {
  module_load_include('inc', 'islandora_oai', 'includes/handler');
  $oai_output = islandora_oai_object_response_xml($params);
  if (is_null($oai_output)) {
    return NULL;
  }

  $dom = new DOMDocument;
  $dom->loadXML($oai_output);

  global $base_url;
  $download_url = $base_url . '/islandora/object/' . $params['pid'] . '/datastream/OBJ/download';

  $identifier = $dom->createElementNS('http://purl.org/dc/elements/1.1/', 'dc:identifier', $download_url);
  $dom->documentElement->appendChild($identifier);

  $modified_oai_output = $dom->saveXML($dom->documentElement);
  return $modified_oai_output;
}

/**
 * Form submit handler.
 */
function islandora_oai_download_link_handler_configuration_submit($form, $form_state) {
  variable_set('islandora_oai_download_link_cmodel_link_mappings', $form_state['values']['islandora_oai_download_links']['islandora_oai_download_link_cmodel_link_mappings']);
  variable_set('islandora_oai_download_link_allowed_ips', $form_state['values']['islandora_oai_download_links']['islandora_oai_download_link_allowed_ips']);
}
