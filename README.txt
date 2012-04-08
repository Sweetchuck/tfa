Two-factor Authentication for Drupal

INSTALLATION:

For default use of the TFA module using the SMS Framework follow these steps.

 * Install SMS Framework module and choose an available gateway
 * Install TFA module
 * Add a user field for storing a user's phone number
 * Visit Administration > Configuration > People > Two-factor Authentication to choose the user field for use

Optionally you can allow certain user roles to skip the TFA process by giving the 'Skip TFA process' permission at Administration > People > Permissions.

Note, if no field is enabled or populated for a user the TFA process will not be started.

API:

Consult tfa.api.php for directions on extending TFA with different channels and storage.
