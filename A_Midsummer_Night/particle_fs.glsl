#version 330 core                                                                    
                                                                                                                                         
uniform sampler2D flame;
uniform sampler2D Round;

in vec2 TexCoord;   
in vec4 fColor;
in float fchoice;
out vec4 FragColor;                                                                 
                                                                                    
void main()                                                                         
{     
    vec4 texColor;
    if(fchoice > 0.5)
        FragColor = texture(flame, TexCoord) * fColor; 
    else
        FragColor = texture(Round, TexCoord) * fColor;     
}
