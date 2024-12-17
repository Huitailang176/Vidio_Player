		EXPORT  get_now_PC

		AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8
			
get_now_PC
    ADR R0, {pc}-0   ; R0 content is current instruction linking address
	BX LR
	
	END
