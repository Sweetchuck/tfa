<?php

class TfaTestTotp extends TfaPlugin implements TfaTotpPlugin {

  public function getForm($form) {
    $form['code'] = array(
      '#type' => 'textfield',
      '#title' => t('Code'),
      '#required' => TRUE,
    );
    $form['login'] = array(
      '#type' => 'submit',
      '#value' => t('Verify'),
    );

    return $form;
  }

  public function validate($code) {
    $this->code = variable_get('tfa_totp', 123456);
    return parent::validate($code);
  }
}
