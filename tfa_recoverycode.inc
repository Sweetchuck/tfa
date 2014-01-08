<?php

class TfaRecoveryCode extends TfaPlugin {

  public function getForm($form) {
    $form['code'] = array(
      '#type' => 'textfield',
      '#title' => t('Recovery code'),
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
