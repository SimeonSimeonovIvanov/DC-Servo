# DC-Servo

![Screen Shot](https://lh3.googleusercontent.com/Xv3BVXqilZLEP8sdvjcjL_cJQlbgq7jir3yXWfOCIcPaj5MP4WAdabvjKhuMbhHNSys4aJBLH0ocGfEHUz21YXoTrse0sBRuv8U6clvefDdncbCatKfYfFrh_Kx5vuFFZ4DfPQbEdlLzkmEeJ9T_reO4QcJ1YtufR4e7pZSFTo8eCXkn3Vm9q_f5xSqGFqwq4Th-pQqi0X41AVXDMtnDtKvc6bQ9O5ntBT4XKXuMc49183EQNKSN-89sdLN8DC8kUnDrx_CRA7-HmtBIoIkLIxqZSrqym2CjSdr3tZ8uEdvRREwooPn_G_ypsy6KOJX9V1KZnun1jotiyMeZuRmrc0tf7F0K4991E0Gq4vfvf7nhF_hq5bJP1iBhqfAUtlfRrkR6PcPBU4gqICn1AHZHvlDfYv6YaeXlGVyWwLQINs_lbGCOn6ULbh-Tn6vfpxfH6Vbh-zhcgoCGGuRA6qg4vJ1bTfgDpJgqDxy31fg-OCgyHtp8wN3fmRdpUpeFoVt2p7yIMgEO0Pnj1alTOSQYytEb8w65kyPlgMPZDyGNiHWiS0Tt14kj-e6zaexdbFDnBoE7L03qxVkj-fuBkQoN7zj6Uq91UEmZELLIqKaAD22daHIdNxC_f-3TtUhBwZbMIde3HB_QVQvQW8zGF2VsTuFKaIptccDrRkv8Ivh0RUTWV-5fBLwe5qC4cWiCU-SL8mJOvRBKjd0wu1zRxA=w1263-h947-no)<br>
![Screen Shot](https://lh3.googleusercontent.com/8H75xgIba8NfEHbxRjB1vH5L4gFnyypGXjzJJ4FnqissgkqRR1jED1oAVVKYW1irBBSW7dX9zWwABgqFDOacOuICHrz5z6lwR_7vQ9Z6tNkFcHOUY45h3Q94_kpth4GqoXRhvNu-ro8Y8QmorKqQvJ6_oS3AN3_AQ_jG1cFxIt2dSuvc23_ynu-QB6oaOL43P6iMKNAYFwow9ijiITR-SxTTWsZR6aRmc0MDAbzLmfE59PgU2MbDLvTm_UKx9Y7MogBPGbULcRuYCYw18QaqZsMScpN45qU1khWV1YsLo2Y7tLJmxWy_iB8pZKH0flq7ymTdNHzOy-o7PWwS2osbBetlkBbGMLUJZ_giXeNKo2EnNz5uLiYHyAKDFCTo3wCDv22AWNPjihpoZfbDqqxhqAPWcthj5eF7UD-fK9wICir5DxN--lncXW3acDQ13Z9XipxcGpaHgAw--jP_i_DEatF77S1ebv2JfJDOrBFJtgIl8ZU3Wf5TCunsloBL1PwJU21bXVCgcEJAW9hb-egOoRqREmBNX9eBfehzeFgKztgI13NrhKkiBxX4Cwy1Lu1oYoPaF74zZPQojfuefT10qucwRYG43-qKwF_vHEKXWceeRh91fnDokzKiZsq535pxQTTvBZ48HsF9zbuQHl4q5fz-GmiIwbBQkav9yUbDfQ=w478-h357-no)<br>
[DC Servo - Pulse Testing](https://www.youtube.com/watch?v=2nZXUcoeyds)<br>
[Images](https://photos.google.com/share/AF1QipMzZ-ynxjgMDZ_RqFM_1IZ0rQbRRlL92HL_IAoDkJfIh321sr6dbftCyruxnr1hNQ?key=cER3Vl9FUmRDOUMzNWs2Z3FKUy1qcHFIYlNoUGlB)<br>

http://mcu-bg.com/mcu_site/viewtopic.php?f=22&t=13129

"Здравейте,

Покрай РОБКО 01 започнах разработка на аналогово DC Servo или по-точно серво усилвател управляващ постояннотоков мотор по скорост. Следващата стъпка е добавяне на Motion Controler. За момента преправям готов код но ще мине доста време до първите тестове.

Сервото се състой от два отделни модула (платки):

1) Токов регулатор (Power Stage.sch) – поддържа тока (въртящия момента) на мотора постоянен спрямо заданието от скоростния регулатор. Предпочетох да избегна аналоговия PI регулатор с по-добрия (според мене) токов ШИМ - на практика P регулатор с голям коефициент на усилване.

2) Скоростен регулатор (Speed Reg.sch) – поддържа ъгловата скорост на мотора постоянна спрямо аналогово задание от ±10V. Самия регулатор има Ramp генератор определящ времето за развъртане на двигателя, цифров вход за реверс удобен при свързване с контролер с еднополярен DAC. Заданието за ток е положително, а посоката се задава от скоростния регулатор през схема генерираща мъртво време за отделните рамена на H-моста. Рзбира и един от най-важните възли – токоограничение. Подходящ в приложения изискващи регулиране на максималния въртящ момент.

Прикачам архив съдържащ две версии на сервото:
1) v.0.0.0 – по нея е изработен прототипа но има множество забележки.
2) v.0.0.1 – в схемата са отстранени голяма част грешките / пропуските от v.0.0.0 но не са довършени платките.

Актуални са EAGLE 5.11 файловете и PDF съдържащи схемите. Архива съдържа и CircuitMaker 2000 симулации на отделните възли.

В показаният playlist се вижда преходната характеристика на скоростния регулатор при различни задания:

DC Servo - Playlist

При проектирането съм ползвал схемни решения от тиристорни регулатори КЕМТОР, КЕМРОС и други. Дължа благодарности на работодателите ми от ЕЛСИ ООД за споделения опит в областта на тиристорните задвижвания.

Поздрави,
Симеон Иванов

РУСЕ,
2014г."

Analog Current PI Loop, FB signal:
![Screen Shot](https://raw.githubusercontent.com/SimeonSimeonovIvanov/DC-Servo/master/Work/Simulation/Current%20PI%20Loop/Current%20PI%20Loop%20(Test%201)%20-%20FB.png)<br>

Current PWM (Digital):
![Screen Shot](https://raw.githubusercontent.com/SimeonSimeonovIvanov/DC-Servo/master/Work/Simulation/CURRENT%20PWM/CURRENT%20PWM%20and%20DC%20Motor.png)<br>
