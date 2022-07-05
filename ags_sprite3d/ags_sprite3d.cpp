#define THIS_IS_THE_PLUGIN

#include <list>
#include "Common.h"
#include "BaseObject.h"
#include "SpriteObject.h"

// TODO: Capsien tarkastus täsmällisten virheilmoitusten saamiseksi
// TODO: "Destroy on room change", "reload on room load"
// TODO: Kahden potenssit tekstuurit!
// TODO: Parenttaaminen (local vs global rotation)
// TODO: Quadin muuntelu -> deformaatiot (Calin)
// TODO: Maskaus multiteksturoinnilla

// Change list (1.1):
// - Added D3D.OpenBackground( int frame )
// - Tinting and transparency
// - Limited parenting: position, rotation, scaling, tint, alpha. Rotating a child sprite doesn't work as local rotation, it adds to the global rotation.
// - Fixed float values


// AGS:n float-tyypin muunnokset
#define SCRIPT_FLOAT(x) long __script_float##x
#define INIT_SCRIPT_FLOAT(x) float x = *((float*)&__script_float##x)
#define FLOAT_RETURN_TYPE long
#define RETURN_FLOAT(x) return *((long*)&x)


IAGSEngine* engine = NULL;

IAGSEngine* GetAGS()
{
    return engine;
}


// DllMain - standard Windows DLL entry point.
// The AGS editor will cause this to get called when the editor first
// starts up, and when it shuts down at the end.
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved )
{
        switch (ul_reason_for_call)
        {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
        }

        return TRUE;
}

// ***** DESIGN TIME CALLS *******

#define IMPORT_D3DOBJECT_BASE \
    "   import attribute bool isEnabled;\r\n"\
    "   import attribute bool isVisible;\r\n"\
    "   import attribute int x;\r\n"\
    "   import attribute int y;\r\n"\
    "   readonly import attribute int width;\r\n"\
    "   readonly import attribute int height;\r\n"\
    "   import attribute float anchorX;\r\n"\
    "   import attribute float anchorY;\r\n"\
    "   import attribute float rotation;\r\n"\
    "   import attribute float scaling;\r\n"\
	"	import attribute float tintR;\r\n"\
	"	import attribute float tintG;\r\n"\
	"	import attribute float tintB;\r\n"\
	"	import attribute float alpha;\r\n"\
    "   import attribute bool isAutoUpdated;\r\n"\
    "   import attribute bool isAutoRendered;\r\n"\
    "   import attribute D3D_RenderStage renderStage;\r\n"\
    "   import attribute D3D_RelativeTo relativeTo;\r\n"\
    "   import attribute int room;\r\n"\
    "   import void SetPosition( int x, int y );\r\n"\
    "   import void SetAnchor( float x, float y );\r\n"\
	"	import void SetTint( float r, float g, float b );\r\n"\
	"	import void SetParent( int parentKey );\r\n"\
	"	import int GetKey();\r\n"\
    "   import void Update();\r\n"\
    "   import void Render();\r\n"

IAGSEditor *editor;
const char *ourScriptHeader =
    
    // *** D3D_Filtering ***
    "enum D3D_Filtering\r\n"
    "{\r\n"
    "   eD3D_FilterNearest = 0,\r\n"
    "   eD3D_FilterLinear = 1\r\n"
    "};\r\n\r\n"

    // *** D3D_RenderStage ***
    "enum D3D_RenderStage\r\n"
    "{\r\n"
    "   eD3D_StageBackground = 0,\r\n"
    "   eD3D_StageScene = 1,\r\n"
    "   eD3D_StageGUI = 2,\r\n"
    "   eD3D_StageScreen = 3,\r\n"
    "};\r\n\r\n"

    // *** D3D_RelativeTo ***
    "enum D3D_RelativeTo\r\n"
    "{\r\n"
    "   eD3D_RelativeToRoom = 0,\r\n"
    "   eD3D_RelativeToScreen = 1\r\n"
    "};\r\n\r\n"

    // *** D3D_Video ***
    "managed struct D3D_Video\r\n"
    "{\r\n"
    
    IMPORT_D3DOBJECT_BASE

        // D3DVideoObject    
    "   import attribute bool isLooping;\r\n"
    "   import attribute float fps;\r\n"

    "   import bool NextFrame();\r\n"
    "   import void Autoplay();\r\n"
    "   import bool IsAutoplaying();\r\n"
    "   import void StopAutoplay();\r\n"
    "};\r\n\r\n"

    // *** D3D_Sprite ***
    "managed struct D3D_Sprite\r\n"
    "{\r\n"

    IMPORT_D3DOBJECT_BASE

        // SpriteObject
    "};\r\n\r\n"
    
    // *** D3D ****
    "struct D3D\r\n"
    "{\r\n"
    "   import static void SetLoopsPerSecond( int loops );\r\n"
    "   import static D3D_Video* OpenVideo( String filename );\r\n"
    "   import static D3D_Sprite* OpenSprite( int graphic );\r\n"
    "   import static D3D_Sprite* OpenSpriteFile( String filename, D3D_Filtering filtering );\r\n"
	"	import static D3D_Sprite* OpenBackground( int frame );\r\n"
    "};\r\n"
	"import void testCall();\r\n"
    ;

LPCSTR AGS_GetPluginName()
{
    // Return the plugin description
    return "Direct3D Plugin";
}

int AGS_EditorStartup( IAGSEditor *lpEditor )
{
    // User has checked the plugin to use it in their game

    // If it's an earlier version than what we need, abort.
    if ( lpEditor->version < 1 )
        return -1;

    editor = lpEditor;
    editor->RegisterScriptHeader( ourScriptHeader );

    // Return 0 to indicate success
    return 0;
}

void AGS_EditorShutdown()
{
    // User has un-checked the plugin from their game
    editor->UnregisterScriptHeader( ourScriptHeader );
}

void AGS_EditorProperties( HWND parent )
{
    // User has chosen to view the Properties of the plugin
    // We could load up an options dialog or something here instead
    MessageBox( parent,
        L"Direct3D Plugin © 2012 Aki Ahonen\n\n"
        L"See ags_d3d.htm for more information.",
        L"About", MB_OK | MB_ICONINFORMATION );
    //MessageBoxA( parent, ourScriptHeader, "About", MB_OK | MB_ICONINFORMATION );
}

int AGS_EditorSaveGame( char *buffer, int bufsize )
{
    // We don't want to save any persistent data
    return 0;
}

void AGS_EditorLoadGame( char *buffer, int bufsize )
{
    // Nothing to load for this dummy plugin
}

// ******* END DESIGN TIME  *******


// ****** RUN TIME ********

#include "D3D9Factory.h"
#include "OGLFactory.h"

Screen screen;
RenderFactory* factory = nullptr;

Screen const* GetScreen()
{
    return &screen;
}

RenderFactory* CreateFactory(const char* driverid)
{
    if (stricmp(driverid, "d3d9") == 0)
    {
        factory = new D3D9Factory();
        return factory;
    }
    else if (stricmp(driverid, "ogl") == 0)
    {
        factory = new OGLFactory();
        return factory;
    }
    return nullptr;
}

RenderFactory* GetFactory()
{
    return factory;
}

std::list< BaseObject* > manualRenderBatch;

// *** D3D ***
void D3D_SetGameSpeed( long speed )
{
    screen.gameSpeed = speed;
    screen.frameDelay = 1.f / speed;
}

SpriteObject_Manager spriteObjManager;

struct D3DVideoObject; // dummy
D3DVideoObject* D3D_OpenVideo( char const* filename )
{
    return nullptr;
}

SpriteObject* D3D_OpenSprite( long spriteID )
{
    SpriteObject* obj = SpriteObject::Open( spriteID );

    if ( obj )
    {
        engine->RegisterManagedObject( obj, &spriteObjManager );
    }

    return obj;
}

int const FILTER_NEAREST = 0;
int const FILTER_LINEAR = 1;

SpriteObject* D3D_OpenSpriteFile( char const* filename, long filtering )
{
    char buffer[MAX_PATH];
    engine->GetPathToFileInCompiledFolder( filename, buffer );

    SpriteObject* obj = SpriteObject::Open( buffer, (BaseObject::Filtering)filtering );

    if ( obj )
    {
        engine->RegisterManagedObject( obj, &spriteObjManager );
    }

    return obj;
}

SpriteObject* D3D_OpenBackground( long frame )
{
    SpriteObject* obj = SpriteObject::OpenBackground( frame );

    if ( obj )
    {
        engine->RegisterManagedObject( obj, &spriteObjManager );
    }

    return obj;
}


// *** BaseObject ***
void D3DObject_SetEnabled( BaseObject* obj, bool enabled ) { obj->SetEnabled( enabled ); }
long D3DObject_GetEnabled( BaseObject* obj ) { return obj->IsEnabled(); }
void D3DObject_SetVisible( BaseObject* obj, bool visible ) { obj->SetVisible( visible ); }
long D3DObject_GetVisible( BaseObject* obj ) { return obj->IsVisible(); }
void D3DObject_SetX( BaseObject* obj, long x ) { long y = obj->GetPosition().y; obj->SetPosition( Point( x, y ) ); }
long D3DObject_GetX( BaseObject* obj ) { return obj->GetPosition().x; }
void D3DObject_SetY( BaseObject* obj, long y ) { long x = obj->GetPosition().x; obj->SetPosition( Point( x, y ) ); }
long D3DObject_GetY( BaseObject* obj ) { return obj->GetPosition().y; }
long D3DObject_GetWidth( BaseObject* obj ) { return obj->GetWidth(); }
long D3DObject_GetHeight( BaseObject* obj ) { return obj->GetHeight(); }

void D3DObject_SetAnchorX( BaseObject* obj, SCRIPT_FLOAT(x) ) {
	INIT_SCRIPT_FLOAT( x );
	float y = obj->GetAnchor().y; obj->SetAnchor( PointF( x, y ) ); }
FLOAT_RETURN_TYPE D3DObject_GetAnchorX( BaseObject* obj ) {
	float x = obj->GetAnchor().x;
	RETURN_FLOAT( x ); }

void D3DObject_SetAnchorY( BaseObject* obj, SCRIPT_FLOAT(y) ) {
	INIT_SCRIPT_FLOAT( y );
	float x = obj->GetAnchor().x; obj->SetAnchor( PointF( x, y ) ); }
FLOAT_RETURN_TYPE D3DObject_GetAnchorY( BaseObject* obj ) {
	float y = obj->GetAnchor().y;
	RETURN_FLOAT( y ); }

void D3DObject_SetRotation( BaseObject* obj, SCRIPT_FLOAT(rot) ) {
	INIT_SCRIPT_FLOAT( rot );
	obj->SetRotation( rot ); }
FLOAT_RETURN_TYPE D3DObject_GetRotation( BaseObject* obj ) {
	float r = obj->GetRotation();
	RETURN_FLOAT( r ); }

void D3DObject_SetScaling( BaseObject* obj, SCRIPT_FLOAT(scaling) ) {
	INIT_SCRIPT_FLOAT( scaling );
	obj->SetScaling( scaling ); }
FLOAT_RETURN_TYPE D3DObject_GetScaling( BaseObject* obj ) {
	float s = obj->GetScaling().x;
	RETURN_FLOAT( s ); }

void D3DObject_SetTintR( BaseObject* obj, SCRIPT_FLOAT(r) ) {
	INIT_SCRIPT_FLOAT( r );
	obj->SetTintR( r ); }
FLOAT_RETURN_TYPE D3DObject_GetTintR( BaseObject* obj ) {
	float r = obj->GetTintR();
	RETURN_FLOAT( r ); }

void D3DObject_SetTintG( BaseObject* obj, SCRIPT_FLOAT(r) ) {
	INIT_SCRIPT_FLOAT( r );
	obj->SetTintG( r ); }
FLOAT_RETURN_TYPE D3DObject_GetTintG( BaseObject* obj ) {
	float g = obj->GetTintG();
	RETURN_FLOAT( g ); }

void D3DObject_SetTintB( BaseObject* obj, SCRIPT_FLOAT(r) ) {
	INIT_SCRIPT_FLOAT( r );
	obj->SetTintB( r ); }
FLOAT_RETURN_TYPE D3DObject_GetTintB( BaseObject* obj ) {
	float b = obj->GetTintB();
	RETURN_FLOAT( b ); }

void D3DObject_SetAlpha( BaseObject* obj, SCRIPT_FLOAT(a) ) {
	INIT_SCRIPT_FLOAT( a ); obj->SetAlpha( a ); }
FLOAT_RETURN_TYPE D3DObject_GetAlpha( BaseObject* obj ) {
	float a = obj->GetAlpha();
	RETURN_FLOAT( a ); }

void D3DObject_SetAutoUpdated( BaseObject* obj, bool autoUpdated ) { obj->SetAutoUpdated( autoUpdated ); }
long D3DObject_GetAutoUpdated( BaseObject* obj ) { return obj->IsAutoUpdated(); }
void D3DObject_SetAutoRendered( BaseObject* obj, bool autoRendered ) { obj->SetAutoRendered( autoRendered ); }
long D3DObject_GetAutoRendered( BaseObject* obj ) { return obj->IsAutoRendered(); }
void D3DObject_SetRenderStage( BaseObject* obj, long stage ) { obj->SetRenderStage( (BaseObject::RenderStage)stage ); }
long D3DObject_GetRenderStage( BaseObject* obj ) { return (long)obj->GetRenderStage(); }
void D3DObject_SetRelativeTo( BaseObject* obj, long relative ) { obj->SetRelativeTo( (BaseObject::RelativeTo)relative ); }
long D3DObject_GetRelativeTo( BaseObject* obj ) { return (long)obj->GetRelativeTo(); }
void D3DObject_SetRoom( BaseObject* obj, long room ) { obj->SetRoom( room ); }
long D3DObject_GetRoom( BaseObject* obj ) { return obj->GetRoom(); }
void D3DObject_SetPosition( BaseObject* obj, long x, long y ) { obj->SetPosition( Point( x, y ) ); }

void D3DObject_SetAnchor( BaseObject* obj, SCRIPT_FLOAT(x), SCRIPT_FLOAT(y) ) {
	INIT_SCRIPT_FLOAT( x );
	INIT_SCRIPT_FLOAT( y );
	obj->SetAnchor( PointF( x, y ) ); }

void D3DObject_SetTint( BaseObject* obj, SCRIPT_FLOAT(r), SCRIPT_FLOAT(g), SCRIPT_FLOAT(b) ) {
	INIT_SCRIPT_FLOAT( r );
	INIT_SCRIPT_FLOAT( g );
	INIT_SCRIPT_FLOAT( b );
	obj->SetTint( r, g, b ); }

void D3DObject_SetParent( BaseObject* obj, int key ) { obj->SetParent( (BaseObject*)GetAGS()->GetManagedObjectAddressByKey( key ) ); }
int D3DObject_GetKey( BaseObject* obj ) { return GetAGS()->GetManagedObjectKeyByAddress( (char*)obj ); }
void D3DObject_Update( BaseObject* obj ) { obj->Update(); }
void D3DObject_Render( BaseObject* obj ) { manualRenderBatch.push_back( obj ); }

// *** D3DVideoObject ***
void D3DVideoObject_SetLooping( D3DVideoObject* obj, bool loop ) { /* dummy */ }
long D3DVideoObject_GetLooping( D3DVideoObject* obj ) { return 0 /* dummy */; }

void D3DVideoObject_SetFPS( D3DVideoObject* obj, SCRIPT_FLOAT(fps) ) {
	INIT_SCRIPT_FLOAT( fps );
    /* dummy */
}
FLOAT_RETURN_TYPE D3DVideoObject_GetFPS( D3DVideoObject* obj ) {
    /* dummy */
	return 0;
}

long D3DVideoObject_NextFrame( D3DVideoObject* obj ) { return 0; /* dummy */ }
void D3DVideoObject_Autoplay( D3DVideoObject* obj ) { /* dummy */; }
long D3DVideoObject_IsAutoplaying( D3DVideoObject* obj ) { return 0; /* dummy */ }
void D3DVideoObject_StopAutoplay( D3DVideoObject* obj ) { /* dummy */ }


void dummy( BaseObject* obj ) {}

#define REG_D3DOBJECT_BASE( cname ) \
    REG( cname "::set_isEnabled", D3DObject_SetEnabled );\
    REG( cname "::get_isEnabled", D3DObject_GetEnabled );\
    REG( cname "::set_isVisible", D3DObject_SetVisible );\
    REG( cname "::get_isVisible", D3DObject_GetVisible );\
    REG( cname "::set_x", D3DObject_SetX );\
    REG( cname "::get_x", D3DObject_GetX );\
    REG( cname "::set_y", D3DObject_SetY );\
    REG( cname "::get_y", D3DObject_GetY );\
    REG( cname "::get_width", D3DObject_GetWidth );\
    REG( cname "::set_width", dummy );\
    REG( cname "::get_height", D3DObject_GetHeight );\
    REG( cname "::set_height", dummy );\
    REG( cname "::set_anchorX", D3DObject_SetAnchorX );\
    REG( cname "::get_anchorX", D3DObject_GetAnchorX );\
    REG( cname "::set_anchorY", D3DObject_SetAnchorY );\
    REG( cname "::get_anchorY", D3DObject_GetAnchorY );\
    REG( cname "::set_rotation", D3DObject_SetRotation );\
    REG( cname "::get_rotation", D3DObject_GetRotation );\
    REG( cname "::set_scaling", D3DObject_SetScaling );\
    REG( cname "::get_scaling", D3DObject_GetScaling );\
	REG( cname "::set_tintR", D3DObject_SetTintR );\
	REG( cname "::get_tintR", D3DObject_GetTintR );\
	REG( cname "::set_tintG", D3DObject_SetTintG );\
	REG( cname "::get_tintG", D3DObject_GetTintG );\
	REG( cname "::set_tintB", D3DObject_SetTintB );\
	REG( cname "::get_tintB", D3DObject_GetTintB );\
	REG( cname "::set_alpha", D3DObject_SetAlpha );\
	REG( cname "::get_alpha", D3DObject_GetAlpha );\
    REG( cname "::set_isAutoUpdated", D3DObject_SetAutoUpdated );\
    REG( cname "::get_isAutoUpdated", D3DObject_GetAutoUpdated );\
    REG( cname "::set_isAutoRendered", D3DObject_SetAutoRendered );\
    REG( cname "::get_isAutoRendered", D3DObject_GetAutoRendered );\
    REG( cname "::set_renderStage", D3DObject_SetRenderStage );\
    REG( cname "::get_renderStage", D3DObject_GetRenderStage );\
    REG( cname "::set_relativeTo", D3DObject_SetRelativeTo );\
    REG( cname "::get_relativeTo", D3DObject_GetRelativeTo );\
    REG( cname "::set_room", D3DObject_SetRoom );\
    REG( cname "::get_room", D3DObject_GetRoom );\
    REG( cname "::SetPosition^2", D3DObject_SetPosition );\
    REG( cname "::SetAnchor^2", D3DObject_SetAnchor );\
	REG( cname "::SetTint^3", D3DObject_SetTint );\
	REG( cname "::SetParent^1", D3DObject_SetParent );\
	REG( cname "::GetKey^0", D3DObject_GetKey );\
    REG( cname "::Update^0", D3DObject_Update );\
    REG( cname "::Render^0", D3DObject_Render )

#define REG( name, func ) { engine->RegisterScriptFunction( name, func ); }


void testCall()
{
	void(*func)( const char*, ... ) = (void(*)( const char*, ... ))engine->GetScriptFunctionAddress( "Display" );
	
	DBG( "%x", (int)engine->GetScriptFunctionAddress( "Character::Say^0" ) );
	func( "%d", (int)engine->GetScriptFunctionAddress( "Character::Say^3" ) );

	DBG( "%x", (int)engine->GetScriptFunctionAddress( "Character::LockView^1" ) );
	func( "%d", (int)engine->GetScriptFunctionAddress( "Character::LockView^1" ) );
}

void AGS_EngineStartup( IAGSEngine *lpEngine )
{
    engine = lpEngine;

    OPEN_DBG( "debug.txt" );
    DBG( "Register" );

    // Make sure it's got the version with the features we need
    DBG("Engine interface: %d", engine->version);
    if (engine->version < 23)
    {
        DBG( "Abort" );
        engine->AbortGame( "Engine interface is too old, need version of AGS with interface version 25 or higher." );
        return;
    }

    engine->RequestEventHook( AGSE_SAVEGAME );
    engine->RequestEventHook( AGSE_RESTOREGAME );
    engine->RequestEventHook( AGSE_PRERENDER );
    engine->RequestEventHook( AGSE_PRESCREENDRAW );
    engine->RequestEventHook( AGSE_PREGUIDRAW );
    engine->RequestEventHook( AGSE_POSTSCREENDRAW );
    engine->RequestEventHook( AGSE_FINALSCREENDRAW );

    // Lukijat
    engine->AddManagedObjectReader( spriteObjManager.GetType(), &spriteObjManager );

    // D3D
    engine->RegisterScriptFunction( "D3D::SetLoopsPerSecond", D3D_SetGameSpeed );
    engine->RegisterScriptFunction( "D3D::OpenVideo", D3D_OpenVideo );
    engine->RegisterScriptFunction( "D3D::OpenSprite", D3D_OpenSprite );
    engine->RegisterScriptFunction( "D3D::OpenSpriteFile", D3D_OpenSpriteFile );
	engine->RegisterScriptFunction( "D3D::OpenBackground", D3D_OpenBackground );
    
    // D3DVideo
    REG_D3DOBJECT_BASE( "D3D_Video" );
    REG( "D3D_Video::set_isLooping", D3DVideoObject_SetLooping );
    REG( "D3D_Video::get_isLooping", D3DVideoObject_GetLooping );
    REG( "D3D_Video::set_fps", D3DVideoObject_SetFPS );
    REG( "D3D_Video::get_fps", D3DVideoObject_GetFPS );
    
    REG( "D3D_Video::NextFrame^0", D3DVideoObject_NextFrame );
    REG( "D3D_Video::Autoplay^0", D3DVideoObject_Autoplay );
    REG( "D3D_Video::IsAutoplaying^0", D3DVideoObject_IsAutoplaying );
    REG( "D3D_Video::StopAutoplay^0", D3DVideoObject_StopAutoplay );
    
    // D3DSprite
    REG_D3DOBJECT_BASE( "D3D_Sprite" );

	REG( "testCall", testCall );
    
    DBG( "Startup" );
}

void AGS_EngineInitGfx( char const* driverID, void* data )
{
    if (!CreateFactory(driverID))
    {
        std::string msg = "Unable to initialize plugin: graphics renderer not supported (";
        msg += driverID; msg += ").";
        engine->AbortGame(msg.c_str());
        return;
    }

    GetFactory()->InitGfxMode(&screen, data);
    //DBG( "Running at %dx%dx%d", screen.backBufferWidth, screen.backBufferHeight, screen.bpp );
}

void AGS_EngineShutdown()
{
    // Dispose any resources and objects
    DBG( "Shutting down" );

    delete factory;
    factory = nullptr;

    CLOSE_DBG();
}

void Save( int handle )
{
    // Screen
    DBG( "SAVE frameDelay: %f", screen.frameDelay );
    engine->FWrite( &screen.frameDelay, sizeof( screen.frameDelay ), handle );
    DBG( "SAVE gameSpeed: %d", screen.gameSpeed );
    engine->FWrite( &screen.gameSpeed, sizeof( screen.gameSpeed ), handle );
}

void Restore( int handle )
{
    // Screen
    engine->FRead( &screen.frameDelay, sizeof( screen.frameDelay ), handle );
    DBG( "RESTORE frameDelay: %f", screen.frameDelay );
    engine->FRead( &screen.gameSpeed, sizeof( screen.gameSpeed ), handle );
    DBG( "RESTORE gameSpeed: %d", screen.gameSpeed );
}

void Render( BaseObject::RenderStage stage )
{
	engine->GetScreenDimensions(&screen.width, &screen.height, &screen.bpp);
    DBG( "RENDER screen %dx%d", screen.width, screen.height );
    // Engine interface >= 25 provides transform matrixes
    if (engine->version >= 25)
    {
        AGSRenderStageDesc desc = {0};
        desc.Version = 25;
        engine->GetRenderStageDesc(&desc);
        GetFactory()->SetScreenMatrixes(&screen, &desc.Matrixes.WorldMatrix, &desc.Matrixes.ViewMatrix, &desc.Matrixes.ProjMatrix);
    }
    else
    {
        GetFactory()->SetScreenMatrixes(&screen, nullptr, nullptr, nullptr);
    }
	
    BaseObject::RenderAll( stage );

    for ( auto i = manualRenderBatch.begin(); i != manualRenderBatch.end(); ++i )
    {
        if ( (*i)->GetRenderStage() == stage )
        {
            (*i)->Render();
        }
    }
}

int AGS_EngineOnEvent( int ev, int data )
{
    if ( engine->IsGamePaused() )
    {
    }

    if ( ev == AGSE_SAVEGAME )
    {
        Save( data );
    }
    else if ( ev == AGSE_RESTOREGAME )
    {
        Restore( data );
    }
    else if ( ev == AGSE_PRERENDER )
    {
        BaseObject::UpdateAll();
    }
    else if ( ev == AGSE_PRESCREENDRAW )
    {
        // Save viewport
        screen.viewport.x = 0;
        screen.viewport.y = 0;
        if (engine->version < 25)
        { // With API 25+ we query transformation matrixes that already tell us this (and more)
            engine->RoomToViewport( &screen.viewport.x, &screen.viewport.y );
        }

        // FIXME: won't work on 64-bit systems!!! use extended engine API?
        GetFactory()->InitGfxDevice(reinterpret_cast<void*>(data));

        Render( BaseObject::STAGE_BACKGROUND );
    }
    else if ( ev == AGSE_PREGUIDRAW )
    {
        Render( BaseObject::STAGE_SCENE );
    }
    else if ( ev == AGSE_POSTSCREENDRAW )
    {
        Render( BaseObject::STAGE_GUI );
    }
    else if ( ev == AGSE_FINALSCREENDRAW )
    {
        Render( BaseObject::STAGE_SCREEN );

        // Clear batch
        manualRenderBatch.clear();
    }

    return 0;
}

// *** END RUN TIME ****
