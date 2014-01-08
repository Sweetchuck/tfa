<?php

class TfaTestFallback extends TfaPlugin {

  public function getForm($form) {
    $form['recover'] = array(
      '#type' => 'textfield',
      '#title' => t('Code'),
      '#required' => TRUE,
    );
    $form['login'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
    );
    return $form;
  }

  public function validate($code) {
    return $code === 'FAILSAFE';
  }
}
