<?php

class TfaTotp extends TfaPlugin implements TfaTotpPlugin {

  public function getForm($form) {
    $form['desc'] = array(
      '#markup' => t('Enter code from your application.'),
    );

    $form['code'] = array(
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
    $this->code = variable_get('tfa_totp', 0) + 1;
    return parent::validate($code);
  }
}
