<?php

class TfaDeviceAuth extends TfaBasePlugin implements TfaLoginPluginInterface {

  protected $trust;

  public function loginAllowed() {
    if ($this->context['uid'] === variable_get('tfa_test_trust', '')) {
      return TRUE;
    }
    return FALSE;
  }

  public function getForm(array $form, array &$form_state) {
    $form['remember_me'] = array(
      '#type' => 'checkbox',
      '#title' => t('Trust this device?'),
    );
    return $form;
  }

  public function submitForm(array $form, array &$form_state) {
    if (isset($form_state['values']['remember_me']) && $form_state['values']['remember_me']) {
      $this->trust = TRUE;
    }
    else {
      $this->trust = FALSE;
    }
  }

  public function finalize() {
    if ($this->trust) {
      variable_set('tfa_test_trust', $this->context['uid']);
    }
  }

}
